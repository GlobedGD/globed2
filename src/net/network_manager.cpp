#include "network_manager.hpp"

#include <data/packets/all.hpp>
#include <managers/error_queues.hpp>
#include <managers/account.hpp>
#include <managers/profile_cache.hpp>
#include <util/net.hpp>
#include <util/debugging.hpp>

using namespace geode::prelude;
using namespace util::data;

NetworkManager::NetworkManager() {
    util::net::initialize();

    if (!gameSocket.create()) util::net::throwLastError();

    // add builtin listeners for connection related packets

    addBuiltinListener<CryptoHandshakeResponsePacket>([this](auto packet) {
        gameSocket.box->setPeerKey(packet->data.key.data());
        _handshaken = true;
        // and lets try to login!
        auto& am = GlobedAccountManager::get();
        std::string authtoken;

        if (!_connectingStandalone) {
            authtoken = *am.authToken.lock();
        }

        auto& pcm = ProfileCacheManager::get();
        pcm.setOwnDataAuto();
        pcm.pendingChanges = false;

        auto gddata = am.gdData.lock();
        auto pkt = LoginPacket::create(gddata->accountId, gddata->accountName, authtoken, pcm.getOwnData());
        this->send(pkt);
    });

    addBuiltinListener<KeepaliveResponsePacket>([](auto packet) {
        GameServerManager::get().finishKeepalive(packet->playerCount);
    });

    addBuiltinListener<ServerDisconnectPacket>([this](auto packet) {
        ErrorQueues::get().error(fmt::format("You have been disconnected from the active server.\n\nReason: <cy>{}</c>", packet->message));
        this->disconnect(true);
    });

    addBuiltinListener<LoggedInPacket>([this](auto packet) {
        log::info("Successfully logged into the server!");
        connectedTps = packet->tps;
        _loggedin = true;
    });

    addBuiltinListener<LoginFailedPacket>([this](auto packet) {
        ErrorQueues::get().error(fmt::format("<cr>Authentication failed!</c> Please try to connect again, if it still doesn't work then reset your authtoken in settings.\n\nReason: <cy>{}</c>", packet->message));
        GlobedAccountManager::get().authToken.lock()->clear();
        this->disconnect(true);
    });

    addBuiltinListener<ServerNoticePacket>([](auto packet) {
        ErrorQueues::get().notice(packet->message);
    });

    addBuiltinListener<ProtocolMismatchPacket>([this](auto packet) {
        std::string message;
        if (packet->serverProtocol < PROTOCOL_VERSION) {
            message = fmt::format(
                "Outdated server! This server uses protocol <cy>v{}</c>, while your client is using protocol <cy>v{}</c>. Downgrade the mod to an older version or ask the server owner to update their server.",
                packet->serverProtocol, PROTOCOL_VERSION
            );
        } else {
            message = fmt::format(
                "Outdated client! Please update the mod to the latest version in order to connect. The server is using protocol <cy>v{}</c>, while this version of the mod only supports protocol <cy>v{}</c>.",
                packet->serverProtocol, PROTOCOL_VERSION
            );
        }

        ErrorQueues::get().error(message);
        this->disconnect(true);
    });

    // boot up the threads

    threadMain.setLoopFunction(&NetworkManager::threadMainFunc);
    threadMain.start(this);

    threadRecv.setLoopFunction(&NetworkManager::threadRecvFunc);
    threadRecv.start(this);
}

NetworkManager::~NetworkManager() {
    log::debug("cleaning up..");

    // clear listeners
    this->removeAllListeners();
    builtinListeners.lock()->clear();

    threadMain.stopAndWait();
    threadRecv.stopAndWait();

    if (this->connected()) {
        log::debug("disconnecting from the server..");
        try {
            this->disconnect(false, true);
        } catch (const std::exception& e) {
            log::warn("error trying to disconnect: {}", e.what());
        }
    }

    util::net::cleanup();

    log::debug("Goodbye!");
}

bool NetworkManager::connect(const std::string& addr, unsigned short port, bool standalone) {
    if (this->connected() && !this->handshaken()) {
        ErrorQueues::get().debugWarn("already trying to connect, please wait");
        return false;
    }

    if (this->connected()) {
        this->disconnect(false);
    }

    _connectingStandalone = standalone;

    lastReceivedPacket = util::time::now();

    if (!standalone) {
        GLOBED_REQUIRE(!GlobedAccountManager::get().authToken.lock()->empty(), "attempting to connect with no authtoken set in account manager")
    }

    GLOBED_REQUIRE(gameSocket.connect(addr, port), "failed to connect to the server")
    gameSocket.createBox();

    auto packet = CryptoHandshakeStartPacket::create(PROTOCOL_VERSION, CryptoPublicKey(gameSocket.box->extractPublicKey()));
    this->send(packet);

    return true;
}

void NetworkManager::connectWithView(const GameServer& gsview) {
    try {
        if (this->connect(gsview.address.ip, gsview.address.port)) {
            GameServerManager::get().setActive(gsview.id);
        }
    } catch(const std::exception& e) {
        this->disconnect(true);
        ErrorQueues::get().error(std::string("Connection failed: ") + e.what());
    }
}

void NetworkManager::connectStandalone() {
    auto server = GameServerManager::get().getServer(GameServerManager::STANDALONE_ID);

    try {
        if (this->connect(server.address.ip, server.address.port, true)) {
            GameServerManager::get().setActive(GameServerManager::STANDALONE_ID);
        }
    } catch (const std::exception& e) {
        this->disconnect(true);
        ErrorQueues::get().error(std::string("Connection failed: ") + e.what());
    }
}

void NetworkManager::disconnect(bool quiet, bool noclear) {
    if (!this->connected()) {
        return;
    }

    if (!quiet) {
        // send it directly instead of pushing to the queue
        gameSocket.sendPacket(DisconnectPacket::create());
    }

    _handshaken = false;
    _loggedin = false;
    _connectingStandalone = false;

    gameSocket.disconnect();
    gameSocket.cleanupBox();

    // GameServerManager could have been destructed before NetworkManager, so this could be UB
    if (!noclear) {
        GameServerManager::get().clearActive();
    }
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
    this->maybeSendKeepalive();

    if (!packetQueue.waitForMessages(util::time::millis(250))) {
        // check for tasks
        if (taskQueue.empty()) return;

        for (const auto& task : taskQueue.popAll()) {
            if (task == NetworkThreadTask::PingServers) {
                auto& sm = GameServerManager::get();
                auto activeServer = sm.active();

                for (auto& [serverId, server] : sm.getAllServers()) {
                    if (serverId == activeServer) continue;

                    try {
                        auto pingId = sm.startPing(serverId);
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

void NetworkManager::threadRecvFunc() {
    if (!gameSocket.poll(1000)) {
        this->maybeDisconnectIfDead();
        return;
    }

    GameSocket::IncomingPacket packet;

    try {
        packet = gameSocket.recvPacket();
    } catch (const std::exception& e) {
        ErrorQueues::get().debugWarn(fmt::format("failed to receive a packet: {}", e.what()));
        return;
    }

    packetid_t packetId = packet.packet->getPacketId();

    if (packetId == PingResponsePacket::PACKET_ID) {
        this->handlePingResponse(packet.packet);
        return;
    }

    // if it's not a ping packet, and it's NOT from the currently connected server, we reject it
    if (!packet.fromServer) {
        return;
    }

    lastReceivedPacket = util::time::now();

    auto builtin = builtinListeners.lock();
    if (builtin->contains(packetId)) {
        (*builtin)[packetId](packet.packet);
        return;
    }

    // this is scary
    geode::Loader::get()->queueInMainThread([this, packetId, packet]() {
        auto listeners_ = this->listeners.lock();
        if (!listeners_->contains(packetId)) {
            ErrorQueues::get().debugWarn(fmt::format("Unhandled packet: {}", packetId));
        } else {
            // xd
            (*listeners_)[packetId](packet.packet);
        }
    });
}

void NetworkManager::handlePingResponse(std::shared_ptr<Packet> packet) {
    if (PingResponsePacket* pingr = dynamic_cast<PingResponsePacket*>(packet.get())) {
        GameServerManager::get().finishPing(pingr->id, pingr->playerCount);
    }
}

void NetworkManager::maybeSendKeepalive() {
    if (_loggedin) {
        auto now = util::time::now();
        if ((now - lastKeepalive) > KEEPALIVE_INTERVAL) {
            lastKeepalive = now;
            this->send(KeepalivePacket::create());
            GameServerManager::get().startKeepalive();
        }
    }
}

// Disconnects from the server if there has been no response for a while
void NetworkManager::maybeDisconnectIfDead() {
    if (!this->connected()) return;

    auto elapsed = util::time::now() - lastReceivedPacket;

    // if we haven't had a handshake response in 5 seconds, assume the server is dead
    if (!this->handshaken() && elapsed > util::time::secs(5)) {
        ErrorQueues::get().error("Failed to connect to the server. No response was received after 5 seconds.");
        this->disconnect(true);
    } else if (elapsed > DISCONNECT_AFTER) {
        ErrorQueues::get().error("The server you were connected to is not responding to any requests. <cy>You have been disconnected.</c>");
        try {
            this->disconnect();
        } catch (const std::exception& e) {
            log::warn("failed to disconnect from a dead server: {}", e.what());
        }
    }
}

void NetworkManager::addBuiltinListener(packetid_t id, PacketCallback callback) {
    (*builtinListeners.lock())[id] = callback;
}

bool NetworkManager::connected() {
    return gameSocket.connected;
}

bool NetworkManager::handshaken() {
    return _handshaken;
}

bool NetworkManager::established() {
    return _loggedin;
}

bool NetworkManager::standalone() {
    return _connectingStandalone;
}