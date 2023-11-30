#include "network_manager.hpp"

#include <data/packets/all.hpp>
#include <managers/error_queues.hpp>
#include <managers/account_manager.hpp>
#include <util/net.hpp>
#include <util/debugging.hpp>

using namespace geode::prelude;
using namespace util::data;

GLOBED_SINGLETON_DEF(NetworkManager)

NetworkManager::NetworkManager() {
    util::net::initialize();

    if (!gameSocket.create()) util::net::throwLastError();

    // add builtin listeners

    addBuiltinListener<CryptoHandshakeResponsePacket>([this](auto packet) {
        gameSocket.box->setPeerKey(packet->data.key.data());
        _established = true;
        // and lets try to login!
        auto& am = GlobedAccountManager::get();
        auto authtoken = *am.authToken.lock();
        auto gddata = am.gdData.lock();
        this->send(LoginPacket::create(gddata->accountId, gddata->accountName, authtoken));
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
        _loggedin = true;
    });

    addBuiltinListener<LoginFailedPacket>([this](auto packet) {
        ErrorQueues::get().error(fmt::format("<cr>Authentication failed!</c> Your credentials have been reset and you have to complete the verification again.\n\nReason: <cy>{}</c>", packet->message));
        GlobedAccountManager::get().authToken.lock()->clear();
        this->disconnect(true);
    });

    addBuiltinListener<ServerNoticePacket>([this](auto packet) {
        ErrorQueues::get().notice(packet->message);
    });

    // boot up the threads

    threadMain = std::thread(&NetworkManager::threadMainFunc, this);
    threadRecv = std::thread(&NetworkManager::threadRecvFunc, this);
}

NetworkManager::~NetworkManager() {
    // clear listeners
    this->removeAllListeners();
    builtinListeners.lock()->clear();

    // wait for threads
    _running = false;

    if (this->connected()) {
        log::debug("disconnecting from the server..");
        this->disconnect();
    }

    log::debug("waiting for threads to halt..");

    if (threadMain.joinable()) threadMain.join();
    if (threadRecv.joinable()) threadRecv.join();

    log::debug("cleaning up..");

    util::net::cleanup();
    log::debug("Goodbye!");
}

void NetworkManager::connect(const std::string& addr, unsigned short port) {
    if (this->connected()) {
        this->disconnect(false);
    }

    lastReceivedPacket = chrono::system_clock::now();

    GLOBED_REQUIRE(!GlobedAccountManager::get().authToken.lock()->empty(), "attempting to connect with no authtoken set in account manager")

    GLOBED_REQUIRE(gameSocket.connect(addr, port), "failed to connect to the server")
    gameSocket.createBox();

    auto packet = CryptoHandshakeStartPacket::create(PROTOCOL_VERSION, CryptoPublicKey(gameSocket.box->extractPublicKey()));
    this->send(packet);
}

void NetworkManager::connectWithView(const GameServerView& gsview) {
    try {
        this->connect(gsview.address.ip, gsview.address.port);
        GlobedServerManager::get().setActiveGameServer(gsview.id);
    } catch (const std::exception& e) {
        this->disconnect(true);
        ErrorQueues::get().error(std::string("Connection failed: ") + e.what());
    }
}

void NetworkManager::disconnect(bool quiet) {
    if (!this->connected()) {
        return;
    }

    if (!quiet) {
        // send it directly instead of pushing to the queue
        gameSocket.sendPacket(DisconnectPacket::create());
    }

    _established = false;
    _loggedin = false;

    gameSocket.disconnect();
    gameSocket.cleanupBox();

    GlobedServerManager::get().setActiveGameServer("");
}

void NetworkManager::send(std::shared_ptr<Packet> packet) {
    GLOBED_REQUIRE(this->connected(), "tried to send a packet while disconnected")
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
        this->maybeSendKeepalive();

        if (!packetQueue.waitForMessages(chrono::milliseconds(250))) {
            // check for tasks
            if (taskQueue.empty()) continue;

            for (const auto& task : taskQueue.popAll()) {
                if (task == NetworkThreadTask::PingServers) {
                    auto& sm = GlobedServerManager::get();
                    auto activeServer = sm.getActiveGameServer();

                    for (auto& [serverId, server] : sm.extractGameServers()) {
                        if (serverId == activeServer) continue;

                        try {
                            auto pingId = sm.pingStart(serverId);
                            gameSocket.sendPacketTo(PingPacket::create(pingId), server.address.ip, server.address.port);
                        } catch (const std::exception& e) {
                            ErrorQueues::get().warn(e.what());
                        }
                    }
                }
            }
        }

        auto messages = packetQueue.popAll();

        for (auto packet : messages) {
            try {
                gameSocket.sendPacket(packet);
            } catch (const std::exception& e) {
                ErrorQueues::get().error(e.what());
            }
        }
    }
}

void NetworkManager::threadRecvFunc() {
    while (_running) {
        auto pollResult = gameSocket.poll(1000);

        if (!pollResult) {
            this->maybeDisconnectIfDead();
            continue;
        }

        std::shared_ptr<Packet> packet;

        try {
            packet = gameSocket.recvPacket();
        } catch (const std::exception& e) {
            ErrorQueues::get().debugWarn(fmt::format("failed to receive a packet: {}", e.what()));
            continue;
        }

        packetid_t packetId = packet->getPacketId();

        if (packetId == PingResponsePacket::PACKET_ID) {
            this->handlePingResponse(packet);
            continue;
        }

        lastReceivedPacket = chrono::system_clock::now();

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

void NetworkManager::handlePingResponse(std::shared_ptr<Packet> packet) {
    if (PingResponsePacket* pingr = dynamic_cast<PingResponsePacket*>(packet.get())) {
        GlobedServerManager::get().pingFinish(pingr->id, pingr->playerCount);
    }
}

void NetworkManager::maybeSendKeepalive() {
    if (_loggedin) {
        auto now = chrono::system_clock::now();
        if ((now - lastKeepalive) > KEEPALIVE_INTERVAL) {
            lastKeepalive = now;
            this->send(KeepalivePacket::create());
            GlobedServerManager::get().pingStartActive();
        }
    }
}

// Disconnects from the server if there has been no response for a while
void NetworkManager::maybeDisconnectIfDead() {
    auto now = chrono::system_clock::now();
    if (this->connected() && (now - lastReceivedPacket) > DISCONNECT_AFTER) {
        ErrorQueues::get().error("The server you were connected to is not responding to any requests. <cy>You have been disconnected.</c>");
        this->disconnect();
    }
}

void NetworkManager::addBuiltinListener(packetid_t id, PacketCallback callback) {
    (*builtinListeners.lock())[id] = callback;
}

bool NetworkManager::connected() {
    return gameSocket.connected;
}

bool NetworkManager::established() {
    return this->connected() && _established;
}

bool NetworkManager::authenticated() {
    return this->established() && _loggedin;
}