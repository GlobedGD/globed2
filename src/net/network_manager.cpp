#include "network_manager.hpp"

#include <util/net.hpp>
#include <data/packets/all.hpp>
#include <managers/server_manager.hpp>
#include <managers/error_queues.hpp>
#include <managers/account_manager.hpp>

namespace log = geode::log;

GLOBED_SINGLETON_DEF(NetworkManager)

NetworkManager::NetworkManager() {
    util::net::initialize();

    if (!socket.create()) util::net::throwLastError();
    if (!pingSocket.create()) util::net::throwLastError();

    // add builtin listeners

    addBuiltinListener<CryptoHandshakeResponsePacket>([this](auto packet) {
        this->socket.box->setPeerKey(packet->data.key.data());
        _established = true;
        // and lets try to login!
        auto& am = GlobedAccountManager::get();
        this->send(LoginPacket::create(am.accountId, *am.authToken.lock()));
    });

    addBuiltinListener<KeepaliveResponsePacket>([this](auto packet) {
        // ?
    });

    addBuiltinListener<ServerDisconnectPacket>([this](auto packet) {
        ErrorQueues::get().error(fmt::format("You have been disconnected from the active server.\n\nReason: <cy>{}</c>", packet->message));
        this->disconnect(true);
    });

    addBuiltinListener<LoggedInPacket>([this](auto packet) {
        this->_loggedin = true;
    });

    // boot up the threads

    threadMain = std::thread(&NetworkManager::threadMainFunc, this);
    threadRecv = std::thread(&NetworkManager::threadRecvFunc, this);
    threadTasks = std::thread(&NetworkManager::threadTasksFunc, this);
}

NetworkManager::~NetworkManager() {
    // clear listeners
    removeAllListeners();
    builtinListeners.lock()->clear();

    // wait for threads
    _running = false;

    if (socket.connected) {
        log::debug("disconnecting from the server..");
        this->disconnect();
    }

    log::debug("waiting for threads to die..");
    if (threadMain.joinable()) threadMain.join();
    if (threadRecv.joinable()) threadRecv.join();
    if (threadTasks.joinable()) threadTasks.join();

    log::debug("cleaning up..");
    for (Packet* packet : packetQueue.popAll()) {
        delete packet;
    }

    util::net::cleanup();
    log::debug("Goodbye!");
}

void NetworkManager::connect(const std::string& addr, unsigned short port) {
    GLOBED_ASSERT(socket.connect(addr, port), "failed to connect to the server")
    socket.createBox();
}

void NetworkManager::disconnect(bool quiet) {
    if (!connected()) {
        return;
    }

    if (!quiet) {
        // send it directly instead of pushing to the queue
        auto pkt = DisconnectPacket::create();
        socket.sendPacket(pkt);
        delete pkt;
    }

    _established = false;
    _loggedin = false;

    socket.disconnect();
    socket.cleanupBox();
}

void NetworkManager::send(Packet* packet) {
    GLOBED_ASSERT(socket.connected, "tried to send a packet while disconnected")
    packetQueue.push(packet);
}

void NetworkManager::addListener(packetid_t id, PacketCallback callback) {
    (*listeners.lock())[id] = callback;
}

void NetworkManager::removeListener(packetid_t id) {
    listeners.lock()->erase(id);
}

void NetworkManager::removeAllListeners() {
    listeners.lock()->clear();
}

// tasks

void NetworkManager::taskPingServers() {
    taskQueue.push(NetworkThreadTask::PingServers);
}

// threads

void NetworkManager::threadMainFunc() {
    while (_running) {
        if (!packetQueue.waitForMessages(std::chrono::seconds(1))) {
            continue;
        }

        auto messages = packetQueue.popAll();

        for (Packet* packet : messages) {
            try {
                socket.sendPacket(packet);
            } catch (const std::exception& e) {
                ErrorQueues::get().error(e.what());
            }

            delete packet;
        }
    }
}

void NetworkManager::threadRecvFunc() {
    while (_running) {
        // we wanna poll both the normal socket and the ping socket.
        auto pollResult = pollBothSockets(1000);
        if (pollResult.hasPing) {
            try {
                auto packet = pingSocket.recvPacket();
                if (PingResponsePacket* pingr = dynamic_cast<PingResponsePacket*>(packet.get())) {
                    GlobedServerManager::get().recordPingResponse(pingr->id, pingr->playerCount);
                }
            } catch (const std::exception& e) {
                ErrorQueues::get().warn(fmt::format("ping error: {}", e.what()));
            }
        }

        if (!pollResult.hasNormal) {
            continue;
        }

        std::shared_ptr<Packet> packet;

        try {
            packet = socket.recvPacket();
        } catch (const std::exception& e) {
            ErrorQueues::get().warn(fmt::format("failed to receive a packet: {}", e.what()));
            continue;
        }

        packetid_t packetId = packet->getPacketId();

        auto builtin = builtinListeners.lock();
        if (builtin->contains(packetId)) {
            (*builtin)[packetId](packet);
            continue;
        }

        // this is scary
        geode::Loader::get()->queueInMainThread([this, packetId, packet]() {
            auto listeners_ = this->listeners.lock();
            if (!listeners_->contains(packetId)) {
                log::warn("Unhandled packet: {}", packetId);
            } else {
                // xd
                (*listeners_)[packetId](packet);
            }
        });
    }
}

void NetworkManager::threadTasksFunc() {
    while (_running) {
        if (!taskQueue.waitForMessages(std::chrono::seconds(1))) {
            continue;
        }

        for (auto task : taskQueue.popAll()) {
            switch (task) {
            case NetworkThreadTask::PingServers: {
                for (auto& [serverId, address] : GlobedServerManager::get().getServerAddresses()) {
                    auto pingId = GlobedServerManager::get().addPendingPing(serverId);
                    Packet* packet = PingPacket::create(pingId);

                    try {
                        pingSocket.connect(address.ip, address.port);
                        pingSocket.sendPacket(packet);
                        pingSocket.disconnect();
                    } catch (const std::exception& e) {
                        ErrorQueues::get().warn(e.what());
                    }

                    delete packet;
                }
            }
            }
        }
    }
}

PollBothResult NetworkManager::pollBothSockets(long msDelay) {
    PollBothResult out;
    if (!socket.connected) {
        // only poll the ping socket in that case
        out.hasNormal = false;
        out.hasPing = pingSocket.poll(msDelay);
        return out;
    }

    GLOBED_SOCKET_POLLFD fds[2];

    fds[0].fd = socket.socket_;
    fds[0].events = POLLIN;

    fds[1].fd = pingSocket.socket_;
    fds[1].events = POLLIN;

    int result = GLOBED_SOCKET_POLL(fds, 2, (int)msDelay);

    if (result == -1) {
        util::net::throwLastError();
    }

    if (result == 0) { // timeout expired
        out.hasNormal = false;
        out.hasPing = false;
    } else {
        // unchecked -- please verify this works in future
        out.hasNormal = (fds[0].revents & POLLIN) != 0;
        out.hasPing = (fds[1].revents & POLLIN) != 0;
    }

    return out;
}

void NetworkManager::addBuiltinListener(packetid_t id, PacketCallback callback) {
    (*builtinListeners.lock())[id] = callback;
}

bool NetworkManager::connected() {
    return socket.connected;
}

bool NetworkManager::established() {
    return socket.connected && _established;
}

bool NetworkManager::authenticated() {
    return socket.connected && _loggedin;
}