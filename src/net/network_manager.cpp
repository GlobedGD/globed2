#include "network_manager.hpp"

#include <data/packets/all.hpp>
#include <managers/server_manager.hpp>
#include <managers/error_queues.hpp>
#include <managers/account_manager.hpp>
#include <util/net.hpp>
#include <util/debugging.hpp>

using namespace geode::prelude;
using namespace util::data;

GLOBED_SINGLETON_DEF(NetworkManager)

NetworkManager::NetworkManager() {
    util::net::initialize();

    if (!socket.create()) util::net::throwLastError();
    if (!pingSocket.create()) util::net::throwLastError();

    // add builtin listeners

    addBuiltinListener<CryptoHandshakeResponsePacket>([this](auto packet) {
        this->socket.box->setPeerKey(packet->data.key.data());
        _established.store(true, std::memory_order::relaxed);
        // and lets try to login!
        auto& am = GlobedAccountManager::get();
        auto authtoken = *am.authToken.lock();
        this->send(LoginPacket::create(am.accountId.load(std::memory_order::relaxed), authtoken));
    });

    addBuiltinListener<KeepaliveResponsePacket>([this](auto packet) {
        GlobedServerManager::get().pingFinishActive(packet->playerCount);
    });

    addBuiltinListener<ServerDisconnectPacket>([this](auto packet) {
        ErrorQueues::get().error(fmt::format("You have been disconnected from the active server.\n\nReason: <cy>{}</c>", packet->message));
        this->disconnect(true);
    });

    addBuiltinListener<LoggedInPacket>([this](auto packet) {
        log::info("Successfully logged into the server!");
        _loggedin.store(true, std::memory_order::relaxed);
    });

    addBuiltinListener<LoginFailedPacket>([this](auto packet) {
        ErrorQueues::get().error(fmt::format("Authentication failed! Please try connecting to the server again.\n\nReason: <cy>{}</c>", packet->message));
        GlobedAccountManager::get().authToken.lock()->clear();
        
        this->disconnect(true);
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
    _running.store(false, std::memory_order::seq_cst);

    if (connected()) {
        log::debug("disconnecting from the server..");
        this->disconnect();
    }

    log::debug("waiting for threads to die..");

    if (threadMain.joinable()) threadMain.join();
    if (threadRecv.joinable()) threadRecv.join();
    if (threadTasks.joinable()) threadTasks.join();

    log::debug("cleaning up..");

    util::net::cleanup();
    log::debug("Goodbye!");
}

void NetworkManager::connect(const std::string& addr, unsigned short port) {
    if (connected()) {
        this->disconnect(false);
    }

    GLOBED_REQUIRE(!GlobedAccountManager::get().authToken.lock()->empty(), "attempting to connect with no authtoken set in account manager")
    
    GLOBED_REQUIRE(socket.connect(addr, port), "failed to connect to the server")
    socket.createBox();

    auto packet = CryptoHandshakeStartPacket::create(PROTOCOL_VERSION, CryptoPublicKey(socket.box->extractPublicKey()));
    this->send(packet);
}

void NetworkManager::disconnect(bool quiet) {
    if (!connected()) {
        return;
    }

    if (!quiet) {
        // send it directly instead of pushing to the queue
        socket.sendPacket(DisconnectPacket::create());
    }

    _established.store(false, std::memory_order::relaxed);
    _loggedin.store(false, std::memory_order::relaxed);

    socket.disconnect();
    socket.cleanupBox();

    GlobedServerManager::get().setActiveGameServer("");
}

void NetworkManager::send(std::shared_ptr<Packet> packet) {
    GLOBED_REQUIRE(socket.connected, "tried to send a packet while disconnected")
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
    while (_running.load(std::memory_order::relaxed)) {
        maybeSendKeepalive();

        if (!packetQueue.waitForMessages(std::chrono::seconds(1))) {
            continue;
        }

        auto messages = packetQueue.popAll();

        for (auto packet : messages) {
            try {
                socket.sendPacket(packet);
            } catch (const std::exception& e) {
                ErrorQueues::get().error(e.what());
            }
        }
    }
}

void NetworkManager::threadRecvFunc() {
    while (_running.load(std::memory_order::relaxed)) {
        // we wanna poll both the normal socket and the ping socket.
        auto pollResult = pollBothSockets(1000);
        if (pollResult.hasPing) {
            try {
                auto packet = pingSocket.recvPacket();
                if (PingResponsePacket* pingr = dynamic_cast<PingResponsePacket*>(packet.get())) {
                    GlobedServerManager::get().pingFinish(pingr->id, pingr->playerCount);
                }
            } catch (const std::exception& e) {
                ErrorQueues::get().warn(fmt::format("ping error: {}", e.what()));
            }
        }

        if (!pollResult.hasNormal) {
            // TODO fix vvv
            // maybeDisconnectIfDead();
            continue;
        }

        lastReceivedPacket = std::chrono::system_clock::now();

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
                ErrorQueues::get().warn(fmt::format("Unhandled packet: {}", packetId));
            } else {
                // xd
                (*listeners_)[packetId](packet);
            }
        });
    }
}

void NetworkManager::threadTasksFunc() {
    while (_running.load(std::memory_order::relaxed)) {
        if (!taskQueue.waitForMessages(std::chrono::seconds(1))) {
            continue;
        }

        for (auto task : taskQueue.popAll()) {
            switch (task) {
            case NetworkThreadTask::PingServers: {
                auto& sm = GlobedServerManager::get();
                auto activeServer = sm.getActiveGameServer();

                for (auto& [serverId, server] : sm.extractGameServers()) {
                    if (serverId == activeServer) continue;

                    try {
                        pingSocket.connect(server.address.ip, server.address.port);
                        auto pingId = sm.pingStart(serverId);
                        pingSocket.sendPacket(PingPacket::create(pingId));
                        pingSocket.disconnect();
                    } catch (const std::exception& e) {
                        ErrorQueues::get().warn(e.what());
                    }
                }
            }
            }
        }
    }
}

void NetworkManager::maybeSendKeepalive() {
    if (_loggedin.load(std::memory_order::relaxed)) {
        auto now = std::chrono::system_clock::now();
        if ((now - lastKeepalive) > KEEPALIVE_INTERVAL) {
            lastKeepalive = now;
            this->send(KeepalivePacket::create());
            GlobedServerManager::get().pingStartActive();
        }
    }
}

// Disconnects from the server if there has been no response for a while
void NetworkManager::maybeDisconnectIfDead() {
    auto now = std::chrono::system_clock::now();
    if ((now - lastReceivedPacket) > DISCONNECT_AFTER) {
        ErrorQueues::get().error("The server you were connected to is not responding to any requests. <cy>You have been disconnected.</c>");
        this->disconnect();
    }
}

PollBothResult NetworkManager::pollBothSockets(long msDelay) {
    PollBothResult out;

    GLOBED_SOCKET_POLLFD fds[2];

    fds[0].fd = socket.socket_.load(std::memory_order::relaxed);
    fds[0].events = POLLIN;

    fds[1].fd = pingSocket.socket_.load(std::memory_order::relaxed);
    fds[1].events = POLLIN;

    int result = GLOBED_SOCKET_POLL(fds, 2, (int)msDelay);

    if (result == -1) {
        util::net::throwLastError();
    }

    if (result == 0) { // timeout expired
        out.hasNormal = false;
        out.hasPing = false;
    } else {
        out.hasNormal = (fds[0].revents & POLLIN) != 0;
        out.hasPing = (fds[1].revents & POLLIN) != 0;
    }

    return out;
}

void NetworkManager::addBuiltinListener(packetid_t id, PacketCallback callback) {
    (*builtinListeners.lock())[id] = callback;
}

bool NetworkManager::connected() {
    return socket.connected.load(std::memory_order::relaxed);
}

bool NetworkManager::established() {
    return connected() && _established.load(std::memory_order::relaxed);
}

bool NetworkManager::authenticated() {
    return established() && _loggedin.load(std::memory_order::relaxed);
}