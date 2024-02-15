#include "network_manager.hpp"

#include <data/packets/all.hpp>
#include <managers/error_queues.hpp>
#include <managers/account.hpp>
#include <managers/profile_cache.hpp>
#include <managers/friend_list.hpp>
#include <managers/settings.hpp>
#include <util/net.hpp>
#include <util/rng.hpp>
#include <util/debug.hpp>

using namespace geode::prelude;
using namespace util::data;

NetworkManager::NetworkManager() {
    util::net::initialize();

    // add builtin listeners for connection related packets

    addBuiltinListener<CryptoHandshakeResponsePacket>([this](auto packet) {
        log::debug("handshake successful, logging in");

        gameSocket.cryptoBox->setPeerKey(packet->data.key.data());
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

        auto& settings = GlobedSettings::get();

        auto gddata = am.gdData.lock();
        auto pkt = LoginPacket::create(this->secretKey, gddata->accountId, gddata->userId, gddata->accountName, authtoken, pcm.getOwnData(), settings.globed.fragmentationLimit);
        this->send(pkt);
    });

    addBuiltinListener<KeepaliveResponsePacket>([](auto packet) {
        GameServerManager::get().finishKeepalive(packet->playerCount);
    });

    addBuiltinListener<KeepaliveTCPResponsePacket>([](auto) {});

    addBuiltinListener<ServerDisconnectPacket>([this](auto packet) {
        this->disconnectWithMessage(packet->message);
    });

    addBuiltinListener<LoggedInPacket>([this](auto packet) {
        log::info("Successfully logged into the server!");
        connectedTps = packet->tps;
        _loggedin = true;

        this->send(ClaimThreadPacket::create(this->secretKey));
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

    addBuiltinListener<AdminAuthSuccessPacket>([this](auto packet) {
        _adminAuthorized = true;
        ErrorQueues::get().success("Successfully authorized");
    });

    // boot up the threads

    threadMain.setLoopFunction(&NetworkManager::threadMainFunc);
    threadMain.setName("Network (out) Thread");
    threadMain.start(this);

    threadRecv.setLoopFunction(&NetworkManager::threadRecvFunc);
    threadRecv.setName("Network (in) Thread");
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

Result<> NetworkManager::connect(const std::string_view addr, unsigned short port, const std::string_view serverId, bool standalone) {
    if (_deferredConnect || (this->connected() && !this->handshaken())) {
        return Err("already trying to connect, please wait");
    }

    if (this->connected()) {
        this->disconnect(false);
    }

    _connectingStandalone = standalone;

    lastReceivedPacket = util::time::now();

    if (!standalone) {
        GLOBED_REQUIRE_SAFE(!GlobedAccountManager::get().authToken.lock()->empty(), "attempting to connect with no authtoken set in account manager")
    }

    _deferredConnect = true;
    _deferredAddr = addr;
    _deferredPort = port;
    _deferredServerId = serverId;
    secretKey = util::rng::Random::get().generate<uint32_t>();

    return Ok();
}

Result<> NetworkManager::connectWithView(const GameServer& gsview) {
    return this->connect(gsview.address.ip, gsview.address.port, gsview.id);
}

Result<> NetworkManager::connectStandalone() {
    auto _server = GameServerManager::get().getServer(GameServerManager::STANDALONE_ID);
    if (!_server.has_value()) {
        return Err(fmt::format("failed to find server by standalone ID"));
    }

    auto server = _server.value();

    return this->connect(server.address.ip, server.address.port, GameServerManager::STANDALONE_ID, true);
}

void NetworkManager::disconnect(bool quiet, bool noclear) {
    _handshaken = false;
    _loggedin = false;
    _connectingStandalone = false;
    _adminAuthorized = false;
    _deferredConnect = false;

    if (!this->connected()) {
        return;
    }

    if (!quiet) {
        // send it directly instead of pushing to the queue
        (void) gameSocket.sendPacket(DisconnectPacket::create());
    }

    gameSocket.disconnect();

    // GameServerManager could have been destructed before NetworkManager, so this could be UB. Additionally will break autoconnect.
    if (!noclear) {
        GameServerManager::get().clearActive();
    }
}

void NetworkManager::disconnectWithMessage(const std::string_view message, bool quiet) {
    ErrorQueues::get().error(fmt::format("You have been disconnected from the active server.\n\nReason: <cy>{}</c>", message));
    this->disconnect(quiet);
}

void NetworkManager::send(std::shared_ptr<Packet> packet) {
    GLOBED_REQUIRE(this->connected(), "tried to send a packet while disconnected")
    packetQueue.push(std::move(packet));
}

void NetworkManager::addListener(packetid_t id, PacketCallback&& callback) {
    (*listeners.lock())[id] = std::move(callback);
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
    if (_suspended) {
        std::this_thread::sleep_for(util::time::millis(250));
        return;
    }

    if (_deferredConnect) {
        auto result = gameSocket.connect(_deferredAddr, _deferredPort);
        _deferredConnect = false;

        if (result) {
            log::debug("tcp connection successful, sending the handshake");
            // successful connection
            GameServerManager::get().setActive(_deferredServerId);
            gameSocket.createBox();

            auto packet = CryptoHandshakeStartPacket::create(PROTOCOL_VERSION, CryptoPublicKey(gameSocket.cryptoBox->extractPublicKey()));
            this->send(packet);
        } else {
            this->disconnect(true);
            ErrorQueues::get().error(fmt::format("Failed to connect: <cy>{}</c>", result.unwrapErr()));
            return;
        }
    }

    this->maybeSendKeepalive();

    if (!packetQueue.waitForMessages(util::time::millis(250))) {
        // check for tasks
        if (taskQueue.empty()) return;

        for (const auto& task : taskQueue.popAll()) {
            if (task == NetworkThreadTask::PingServers) {
                auto& sm = GameServerManager::get();
                auto activeServer = sm.getActiveId();

                for (auto& [serverId, server] : sm.getAllServers()) {
                    if (serverId == activeServer) continue;

                    auto pingId = sm.startPing(serverId);
                    auto result = gameSocket.sendPacketTo(PingPacket::create(pingId), server.address.ip, server.address.port);

                    if (result.isErr()) {
                        ErrorQueues::get().warn(result.unwrapErr());
                    }
                }
            }
        }
    }

    auto messages = packetQueue.popAll();

    for (auto packet : messages) {
        try {
            auto result = gameSocket.sendPacket(packet);
            if (!result) {
                log::debug("failed to send packet {}: {}", packet->getPacketId(), result.unwrapErr());
                this->disconnectWithMessage(result.unwrapErr());
                return;
            }

        } catch (const std::exception& e) {
            log::debug("sending packet failed");
            ErrorQueues::get().error(e.what());
            this->disconnect(true, false);
        }
    }
}

void NetworkManager::threadRecvFunc() {
    if (_suspended || _deferredConnect) {
        std::this_thread::sleep_for(util::time::millis(100));
        return;
    }

    bool fromConnected;
    bool timedOut;

    auto packet_ = gameSocket.recvPacket(100, fromConnected, timedOut);

    if (timedOut) {
        this->maybeDisconnectIfDead();
        return;
    }

    if (packet_.isErr()) {
        ErrorQueues::get().debugWarn(fmt::format("failed to receive a packet: {}", packet_.unwrapErr()));
        this->disconnectWithMessage(packet_.unwrapErr(), true);
        return;
    }

    auto packet = packet_.unwrap();

    packetid_t packetId = packet->getPacketId();

    if (packetId == PingResponsePacket::PACKET_ID) {
        this->handlePingResponse(packet);
        return;
    }

    // if it's not a ping packet, and it's NOT from the currently connected server, we reject it
    if (!fromConnected) {
        return;
    }

    lastReceivedPacket = util::time::now();

    auto builtin = builtinListeners.lock();
    if (builtin->contains(packetId)) {
        (*builtin)[packetId](packet);
        return;
    }

    // this is scary
    Loader::get()->queueInMainThread([this, packetId, packet] {
        auto listeners_ = this->listeners.lock();
        if (!listeners_->contains(packetId)) {
            auto suppressed_ = suppressed.lock();

            if (suppressed_->contains(packetId) && util::time::systemNow() > suppressed_->at(packetId)) {
                suppressed_->erase(packetId);
            }

            if (!suppressed_->contains(packetId)) {
                ErrorQueues::get().debugWarn(fmt::format("Unhandled packet: {}", packetId));
            }
        } else {
            // xd
            (*listeners_)[packetId](packet);
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

        // this is only done to keep the nat hole open
        if ((now - lastTcpKeepalive) > TCP_KEEPALIVE_INTERVAL) {
            lastTcpKeepalive = now;
            this->send(KeepaliveTCPPacket::create());
        }
    }
}

// Disconnects from the server if there has been no response for a while
void NetworkManager::maybeDisconnectIfDead() {
    if (!this->connected()) return;

    auto elapsed = util::time::now() - lastReceivedPacket;

    // if we haven't had a handshake response in 5 seconds, assume the server is dead
    if (!this->handshaken() && elapsed > util::time::seconds(5)) {
        ErrorQueues::get().error("Failed to connect to the server. No response was received after 5 seconds.");
        this->disconnect(true);
    } else if (elapsed > DISCONNECT_AFTER) {
        ErrorQueues::get().error("The server you were connected to is not responding to any requests. <cy>You have been disconnected.</c>");
        try {
            this->disconnect();
        } catch (const std::exception& e) {
            log::warn("failed to disconnect from a dead server: {}", e.what());
            this->disconnect(true);
        }
    }
}

void NetworkManager::addBuiltinListener(packetid_t id, PacketCallback&& callback) {
    (*builtinListeners.lock())[id] = std::move(callback);
}

bool NetworkManager::connected() {
    return gameSocket.isConnected();
}

bool NetworkManager::handshaken() {
    return _handshaken;
}

bool NetworkManager::established() {
    return _loggedin;
}

bool NetworkManager::isAuthorizedAdmin() {
    return _adminAuthorized;
}

bool NetworkManager::standalone() {
    return _connectingStandalone;
}

void NetworkManager::suspend() {
    _suspended = true;
}

void NetworkManager::resume() {
    _suspended = false;
}