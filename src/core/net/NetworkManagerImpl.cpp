#include "NetworkManagerImpl.hpp"
#include <globed/core/ValueManager.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/core/net/NetworkManager.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/data/PlayerDisplayData.hpp>
#include <core/CoreImpl.hpp>
#include "data/helpers.hpp"

#include <argon/argon.hpp>
#include <qunet/Pinger.hpp>
#include <qunet/util/algo.hpp>
#include <asp/time/Duration.hpp>
#include <asp/time/sleep.hpp>

using namespace geode::prelude;
using namespace asp::time;

static qn::ConnectionDebugOptions getConnOpts() {
    qn::ConnectionDebugOptions opts{};
    opts.packetLossSimulation = globed::setting<float>("core.dev.packet-loss-sim");

    if (opts.packetLossSimulation != 0.f) {
        log::warn("Packet loss simulation enabled: {}%", opts.packetLossSimulation * 100.f);
    }

    return opts;
}

static globed::PlayerIconData gatherIconData() {
    auto gm = globed::cachedSingleton<GameManager>();

    globed::PlayerIconData out{};
    out.cube = gm->m_playerFrame;
    out.ship = gm->m_playerShip;
    out.ball = gm->m_playerBall;
    out.ufo = gm->m_playerBird;
    out.wave = gm->m_playerDart;
    out.robot = gm->m_playerRobot;
    out.spider = gm->m_playerSpider;
    out.swing = gm->m_playerSwing;
    out.jetpack = gm->m_playerJetpack;
    out.deathEffect = gm->m_playerDeathEffect;
    out.color1 = gm->m_playerColor;
    out.color2 = gm->m_playerColor2;
    if (gm->m_playerGlow) {
        out.glowColor = gm->m_playerGlowColor.value();
    }
    out.trail = gm->m_playerStreak;
    out.shipTrail = gm->m_playerShipFire;

    return out;
}

static Duration getPingInterval(uint32_t sentPings) {
    switch (sentPings) {
        case 0: return Duration::fromSecs(1);
        case 1: return Duration::fromSecs(2);
        case 2: return Duration::fromSecs(3);
        case 3: return Duration::fromSecs(5);
        case 4: return Duration::fromSecs(10);
        default: return Duration::fromSecs(300); // rare, because we already know the average latency
    }
}

namespace globed {

void GameServer::updateLatency(uint32_t latency) {
    if (avgLatency == -1) {
        avgLatency = latency;
    } else {
        avgLatency = qn::exponentialMovingAverage(avgLatency, latency, 0.2);
    }

    lastLatency = latency;
}


NetworkManagerImpl::NetworkManagerImpl() {
    // TODO: measure how much of an impact those have on bandwidth
    m_centralConn.setActiveKeepaliveInterval(Duration::fromSecs(45));
    m_gameConn.setActiveKeepaliveInterval(Duration::fromSecs(10));

    m_centralConn.setConnectionStateCallback([this](qn::ConnectionState state) {
        m_centralConnState = state;
        if (state == qn::ConnectionState::Connected) {
            this->onCentralConnected();
        } else if (state == qn::ConnectionState::Disconnected) {
            this->onCentralDisconnected();
        }
    });

    m_centralConn.setDataCallback([this](std::vector<uint8_t> bytes) {
        qn::ByteReader breader{bytes};
        size_t unpackedSize = breader.readVarUint().unwrapOr(-1);

        kj::ArrayInputStream ais{{bytes.data() + breader.position(), bytes.size() - breader.position()}};
        capnp::PackedMessageReader reader{ais};

        CentralMessage::Reader msg = reader.getRoot<CentralMessage>();

        if (auto err = this->onCentralDataReceived(msg).err()) {
            log::error("failed to process message from central server: {}", err);
        }
    });

    m_gameConn.setConnectionStateCallback([this](qn::ConnectionState state) {
        m_gameConnState = state;
        if (state == qn::ConnectionState::Connected) {
            log::debug("connected to game server at {}", m_gameServerUrl);
            m_gameEstablished = true;
            m_gameEventQueue = {}; // TODO: move this into a separate function or smth

            // if there was a deferred join, try to login with session, otherwise just login
            if (m_gsDeferredJoin) {
                this->sendGameLoginJoinRequest(*m_gsDeferredJoin);
            } else {
                this->sendGameLoginRequest();
            }
        } else if (state == qn::ConnectionState::Disconnected) {
            log::debug("disconnected from game server at {}", m_gameServerUrl);
            m_gameEstablished = false;
            m_gameServerUrl.clear();

            // if there was a deferred join, try to connect now
            if (m_gsDeferredConnectJoin) {
                m_gameConn.setDebugOptions(getConnOpts());
                auto res = m_gameConn.connect(m_gsDeferredConnectJoin->first);
                m_gameServerUrl = m_gsDeferredConnectJoin->first;

                if (!res) {
                    log::error("Failed to connect to game server {}: {}", m_gsDeferredConnectJoin->first, res.unwrapErr().message());
                    m_gameServerUrl.clear();
                }
            }
        }
    });

    m_gameConn.setDataCallback([this](std::vector<uint8_t> bytes) {
        qn::ByteReader breader{bytes};
        size_t unpackedSize = breader.readVarUint().unwrapOr(-1);

        kj::ArrayInputStream ais{{bytes.data() + breader.position(), bytes.size() - breader.position()}};
        capnp::PackedMessageReader reader{ais};

        GameMessage::Reader msg = reader.getRoot<GameMessage>();

        if (auto err = this->onGameDataReceived(msg).err()) {
            log::error("failed to process message from game server: {}", err);
        }
    });

    m_workerThread.setName("NetworkManager worker");
    m_workerThread.setLoopFunction([this](asp::StopToken<>& stopToken) {
        // check if any game servers need to be pinged
        if (this->isConnected()) {
            auto servers = m_gameServers.lock();

            for (auto& [srvkey, server] : *servers) {
                if (server.lastPingTime.elapsed() > getPingInterval(server.sentPings)) {
                    server.sentPings++;
                    server.lastPingTime = Instant::now();

                    // send a ping to the server
                    auto res = qn::Pinger::get().pingUrl(server.url, [this, srvkey](qn::PingResult res) {
                        if (res.timedOut) {
                            log::debug("Ping to server {} timed out!", srvkey);
                            return;
                        }

                        // update the server latency
                        auto servers = m_gameServers.lock();
                        auto it = servers->find(srvkey);
                        if (it != servers->end()) {
                            auto lat = (uint32_t)res.responseTime.millis();
                            it->second.updateLatency(lat);
                            log::debug("Ping to server {} arrived, latency: {}ms avg, {}ms last", srvkey, it->second.avgLatency, it->second.lastLatency);
                        }
                    });

                    if (!res) {
                        log::warn("Failed to ping game server {} ({}, id {}): {}", server.url, server.name, (int)server.id, res.unwrapErr());
                        continue;
                    }
                }
            }
        }

        asp::time::sleep(Duration::fromMillis(100));
    });
    m_workerThread.start();
}

NetworkManagerImpl::~NetworkManagerImpl() {
    m_workerThread.stopAndWait();
}

NetworkManagerImpl& NetworkManagerImpl::get() {
    return *NetworkManager::get().m_impl;
}

Result<> NetworkManagerImpl::connectCentral(std::string_view url) {
    if (m_centralConn.connected()) {
        return Err("Already connected to central server");
    }

    m_centralUrl = std::string(url);
    m_knownArgonUrl.clear();

    m_centralConn.setDebugOptions(getConnOpts());

    return m_centralConn.connect(url).mapErr([](auto&& err) {
        return err.message();
    });
}

Result<> NetworkManagerImpl::disconnectCentral() {
    if (m_waitingForArgon) {
        return Err("cannot disconnect while waiting for Argon auth");
    }

    (void) m_centralConn.disconnect();

    return Ok();
}

qn::ConnectionState NetworkManagerImpl::getConnState(bool game) {
    return game ? m_gameConnState.load() : m_centralConnState.load();
}

std::optional<uint8_t> NetworkManagerImpl::getPreferredServer() {
    if (!m_established) {
        return std::nullopt;
    }

    auto servers = m_gameServers.lock();

    if (auto value = globed::value<std::string>("core.net.preferred-server")) {
        for (auto& srv : *servers) {
            if (srv.second.stringId == *value) {
                return srv.second.id;
            }
        }
    }

    // fallback to the server with the lowest latency
    if (!servers->empty()) {
        auto it = std::min_element(servers->begin(), servers->end(),
            [](const auto& a, const auto& b) {
                return a.second.lastLatency < b.second.lastLatency;
            });
        return it->second.id;
    }

    return std::nullopt;
}

bool NetworkManagerImpl::isConnected() const {
    return m_established;
}

Duration NetworkManagerImpl::getGamePing() {
    return m_gameConn.getLatency();
}

Duration NetworkManagerImpl::getCentralPing() {
    return m_centralConn.getLatency();
}

void NetworkManagerImpl::onCentralConnected() {
    log::debug("connection to central server established, trying to log in");

    m_established = false;
    this->tryAuth();
}

void NetworkManagerImpl::tryAuth() {
    auto gam = GJAccountManager::get();
    int accountId = gam->m_accountID;
    int userId = GameManager::get()->m_playerUserID;

    if (auto stoken = this->getUToken()) {
        (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
            log::debug("attempting login with user token {}", *stoken);
            auto loginUToken = msg.initLoginUToken();
            loginUToken.setToken(*stoken);
            loginUToken.setAccountId(accountId);
        });
    } else if (!m_knownArgonUrl.empty()) {
        m_waitingForArgon = true;

        (void) argon::setServerUrl(m_knownArgonUrl);
        auto res = argon::startAuth([&](Result<std::string> res) {
            m_waitingForArgon = false;

            if (!res) {
                this->abortConnection(fmt::format("failed to complete Argon auth: {}", res.unwrapErr()));
                return;
            }

            this->doArgonAuth(*res);
        });

        if (!res) {
            m_waitingForArgon = false;
            this->abortConnection(fmt::format("failed to start Argon auth: {}", res.unwrapErr()));
            return;
        }
    } else {
        (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
            log::debug("attempting plain login");
            auto loginPlain = msg.initLoginPlain();
            auto data = loginPlain.initData();
            data.setUsername(gam->m_username);
            data.setAccountId(accountId);
            data.setUserId(userId);
        });
    }
}

void NetworkManagerImpl::doArgonAuth(std::string token) {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        log::debug("attempting login with argon token");
        auto loginUToken = msg.initLoginArgon();
        loginUToken.setToken(token);
        loginUToken.setAccountId(GJAccountManager::get()->m_accountID);
    });
}

void NetworkManagerImpl::abortConnection(std::string reason) {
    log::warn("aborting connection to central server: {}", reason);
    (void) m_centralConn.disconnect();
}

void NetworkManagerImpl::onCentralDisconnected() {
    log::debug("connection to central server lost!");

    CoreImpl::get().onServerDisconnected();

    m_established = false;
    m_gameServers.lock()->clear();
    m_knownArgonUrl.clear();
}

void NetworkManagerImpl::sendJoinSession(SessionId id) {
    if (!m_established) {
        log::error("Cannot join session, not connected to central server");
        return;
    }

    log::debug("Joining session with ID {}", id.asU64());

    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto joinSession = msg.initJoinSession();
        joinSession.setSessionId(id);
    });

    // find the game server
    uint8_t serverId = id.serverId();
    for (auto& srv : *m_gameServers.lock()) {
        if (srv.second.id == serverId) {
            this->joinSessionWith(srv.second.url, id);
            return;
        }
    }

    // not found!
    log::error("Tried joining an invalid session, game server with ID {} does not exist", static_cast<int>(serverId));
}

void NetworkManagerImpl::sendLeaveSession() {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        msg.initLeaveSession();
    });

    (void) this->sendToGame([&](GameMessage::Builder& msg) {
        msg.initLeaveSession();
    });
}

void NetworkManagerImpl::joinSessionWith(std::string_view serverUrl, SessionId id) {
    // if already connected to another game server, disconnect and wait for the connection to close
    if (m_gameConn.connected()) {
        if (m_gameServerUrl != serverUrl) {
            m_gsDeferredConnectJoin = std::make_pair(std::string(serverUrl), id);
            (void) m_gameConn.disconnect();
        } else if (m_gameEstablished) {
            // same server, just send the join request
            this->sendGameJoinRequest(id);
        } else {
            this->sendGameLoginJoinRequest(id);
        }
    } else {
        // not connected, connect to the game server and join later
        m_gsDeferredJoin = id;
        m_gameServerUrl = std::string(serverUrl);
        m_gameConn.setDebugOptions(getConnOpts());
        auto res = m_gameConn.connect(serverUrl);
        if (!res) {
            log::error("Failed to connect to {}: {}", serverUrl, res.unwrapErr().message());
            m_gameServerUrl.clear();
            m_gsDeferredJoin.reset();
        }
    }
}

void NetworkManagerImpl::sendGameLoginJoinRequest(SessionId id) {
    (void) this->sendToGame([&](GameMessage::Builder& msg) {
        auto loginJoin = msg.initLoginUTokenAndJoin();
        loginJoin.setAccountId(GJAccountManager::get()->m_accountID);
        loginJoin.setToken(this->getUToken().value_or(""));
        loginJoin.setSessionId(id);
        loginJoin.setPasscode(0); // TODO

        auto iconBuilder = loginJoin.initIcons();
        data::encodeIconData(gatherIconData(), iconBuilder);
    });
}

void NetworkManagerImpl::sendGameLoginRequest() {
    (void) this->sendToGame([&](GameMessage::Builder& msg) {
        auto login = msg.initLoginUToken();
        login.setAccountId(GJAccountManager::get()->m_accountID);
        login.setToken(this->getUToken().value_or(""));

        auto iconBuilder = login.initIcons();
        data::encodeIconData(gatherIconData(), iconBuilder);
    });
}

void NetworkManagerImpl::sendGameJoinRequest(SessionId id) {
    (void) this->sendToGame([&](GameMessage::Builder& msg) {
        auto join = msg.initJoinSession();
        join.setSessionId(id);
        join.setPasscode(0); // TODO
    });
}

void NetworkManagerImpl::sendPlayerState(const PlayerState& state, const std::vector<int>& dataRequests) {
    (void) this->sendToGame([&](GameMessage::Builder& msg) {
        auto playerData = msg.initPlayerData();
        auto data = playerData.initData();
        data::encodePlayerState(state, data);

        auto reqs = playerData.initDataRequests(dataRequests.size());
        for (size_t i = 0; i < dataRequests.size(); ++i) {
            reqs.set(i, dataRequests[i]);
        }

        auto evs = playerData.initEvents(std::min<size_t>(m_gameEventQueue.size(), 64));
        for (size_t i = 0; i < evs.size(); i++) {
            auto& event = m_gameEventQueue.front();

            auto ev = evs[i];
            ev.setType(event.type);
            ev.setData({(kj::byte*) event.data.data(), event.data.size()});

            m_gameEventQueue.pop();
        }
    }, !m_gameEventQueue.empty());
}

void NetworkManagerImpl::queueGameEvent(Event&& event) {
    if (!m_gameEstablished) return;

    m_gameEventQueue.push(std::move(event));
}

void NetworkManagerImpl::sendRoomStateCheck() {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        msg.setCheckRoomState();
    });
}

void NetworkManagerImpl::sendCreateRoom(const std::string& name, uint32_t passcode, const RoomSettings& settings) {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto createRoom = msg.initCreateRoom();
        createRoom.setName(name);
        createRoom.setPasscode(passcode);

        auto sb = createRoom.initSettings();
        data::encodeRoomSettings(settings, sb);
    });
}

void NetworkManagerImpl::sendJoinRoom(uint32_t id, uint32_t passcode) {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto joinRoom = msg.initJoinRoom();
        joinRoom.setRoomId(id);
        joinRoom.setPasscode(passcode);
    });
}

void NetworkManagerImpl::sendLeaveRoom() {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        msg.setLeaveRoom();
    });
}

void NetworkManagerImpl::sendRequestRoomList() {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto requestRoomList = msg.initRequestRoomList();
    });
}

void NetworkManagerImpl::addListener(const std::type_info& ty, void* listener) {
    std::type_index index{ty};
    auto listeners = m_listeners.lock();
    auto& ls = (*listeners)[index];

    ls.push_back(listener);

    log::debug(
        "Added listener {} for message type '{}', priority: {}",
        listener,
        ty.name(),
        static_cast<MessageListenerImplBase*>(listener)->m_priority
    );

    // sort them by priority
    std::sort(ls.begin(), ls.end(), [](void* a, void* b) {
        auto* implA = static_cast<MessageListenerImplBase*>(a);
        auto* implB = static_cast<MessageListenerImplBase*>(b);
        return implA->m_priority < implB->m_priority;
    });
}

void NetworkManagerImpl::removeListener(const std::type_info& ty, void* listener) {
    std::type_index index{ty};
    auto listeners = m_listeners.lock();

    log::debug(
        "Removing listener {} for message type '{}'",
        listener,
        ty.name()
    );

    auto it = listeners->find(index);
    if (it != listeners->end()) {
        auto& vec = it->second;
        auto pos = std::find(vec.begin(), vec.end(), listener);
        if (pos != vec.end()) {
            vec.erase(pos);
            if (vec.empty()) {
                listeners->erase(it);
            }
        }
    }
}

static void updateServers(std::unordered_map<std::string, GameServer>& servers, auto& newServers) {
    servers.clear();

    for (auto srv : newServers) {
        GameServer gameServer {
            .id = srv.getId(),
            .stringId = srv.getStringId(),
            .url = srv.getAddress(),
            .name = srv.getName(),
            .region = srv.getRegion()
        };

        log::debug("Added game server: {} (ID: {}, URL: {}, Region: {})",
                   gameServer.name, static_cast<int>(gameServer.id), gameServer.url, gameServer.region);

        servers[gameServer.stringId] = std::move(gameServer);
    }
}

Result<> NetworkManagerImpl::onCentralDataReceived(CentralMessage::Reader& msg) {
    switch (msg.which()) {
        case CentralMessage::LOGIN_OK: {
            auto loginOk = msg.getLoginOk();

            if (loginOk.hasNewToken()) {
                std::string newToken = loginOk.getNewToken();
                this->setUToken(newToken);
            }

            if (loginOk.hasServers()) {
                auto servers = loginOk.getServers();
                updateServers(*m_gameServers.lock(), servers);
            }

            m_established = true;
            CoreImpl::get().onServerConnected();
        } break;

        case CentralMessage::LOGIN_FAILED: {
            auto loginFailed = msg.getLoginFailed();
            this->handleLoginFailed(loginFailed.getReason());
        } break;

        case CentralMessage::LOGIN_REQUIRED: {
            auto loginRequired = msg.getLoginRequired();

            if (!loginRequired.hasArgonUrl()) {
                return Err("Login required message does not contain Argon URL");
            }

            m_knownArgonUrl = loginRequired.getArgonUrl();
            this->clearUToken();
            this->tryAuth();
        } break;

        case CentralMessage::PLAYER_COUNTS: {
            // TODO
        } break;

        case CentralMessage::ROOM_STATE: {
            auto roomState = msg.getRoomState();

            this->invokeListeners(data::decodeRoomStateMessage(roomState));
        } break;

        case CentralMessage::ROOM_JOIN_FAILED: {
            auto roomJoinFailed = msg.getRoomJoinFailed();

            if (auto msg = data::decodeRoomJoinFailedMessage(roomJoinFailed)) {
                this->invokeListeners(*msg);
            } else {
                log::warn("Received invalid room join failed message");
            }
        } break;

        case CentralMessage::ROOM_CREATE_FAILED: {
            auto roomCreateFailed = msg.getRoomCreateFailed();

            if (auto msg = data::decodeRoomCreateFailedMessage(roomCreateFailed)) {
                this->invokeListeners(*msg);
            } else {
                log::warn("Received invalid room create failed message");
            }
        } break;

        case CentralMessage::ROOM_LIST: {
            auto roomList = msg.getRoomList();

            if (auto msg = data::decodeRoomListMessage(roomList)) {
                this->invokeListeners(*msg);
            } else {
                log::warn("Received invalid room list message");
            }
        } break;

        case CentralMessage::JOIN_FAILED: {
            log::warn("Received join failed message: {}", (int) msg.getJoinFailed().getReason());
        } break;

        case CentralMessage::WARP_PLAYER: {
            auto warpPlayer = msg.getWarpPlayer();
            auto sessionId = SessionId{warpPlayer.getSession()};

            this->invokeListeners(msg::WarpPlayerMessage{sessionId});
        } break;

        case CentralMessage::KICKED: {
            // TODO
        } break;

        default: {
            return Err("Received unknown message type: {}", std::to_underlying(msg.which()));
        } break;
    }

    return Ok();
}

Result<> NetworkManagerImpl::onGameDataReceived(GameMessage::Reader& msg) {
    switch (msg.which()) {
        case GameMessage::LOGIN_OK: {
            m_gameEstablished = true;
            log::debug("Successfully logged in to game server");
        } break;

        case GameMessage::LOGIN_FAILED: {
            using enum schema::game::LoginFailedReason;

            auto reason = msg.getLoginFailed().getReason();

            switch (reason) {
                case INVALID_USER_TOKEN: {
                    log::warn("Login failed: invalid user token, aborting central connection to re-auth");
                    this->abortConnection("re-authentication required");
                } break;

                case CENTRAL_SERVER_UNREACHABLE: {
                    log::warn("Login failed: central server unreachable, aborting connection");
                    // TODO: show to the user somehow
                } break;
            }

            (void) m_gameConn.disconnect();
        } break;

        case GameMessage::JOIN_SESSION_OK: {
            // TODO: post listener message
        } break;

        case GameMessage::JOIN_SESSION_FAILED: {
            using enum schema::game::JoinSessionFailedReason;

            auto reason = msg.getJoinSessionFailed().getReason();

            // TODO: post listener message

            switch (reason) {
                case INVALID_PASSCODE: {
                    log::warn("Join session failed: invalid passcode");
                } break;

                case INVALID_ROOM: {
                    log::warn("Join session failed: invalid room");
                } break;
            }
        } break;

        case GameMessage::LEVEL_DATA: {
            auto data = msg.getLevelData();
            this->invokeListeners(data::decodeLevelDataMessage(data));
        } break;

        case GameMessage::KICKED: {
            // TODO
        } break;

        default: {
            return Err("Received unknown message type: {}", std::to_underlying(msg.which()));
        } break;
    }

    return Ok();
}

void NetworkManagerImpl::handleLoginFailed(schema::main::LoginFailedReason reason) {
    using enum schema::main::LoginFailedReason;

    switch (reason) {
        case INVALID_USER_TOKEN: {
            log::warn("Login failed: invalid user token, clearing token and trying to re-auth");
            this->clearUToken();
            this->tryAuth();
        } break;

        case INVALID_ARGON_TOKEN: {
            log::warn("Login failed: invalid Argon token, clearing token and trying to re-auth");
            argon::clearToken();
            this->tryAuth();
        } break;

        case ARGON_NOT_SUPPORTED: {
            log::warn("Login failed: Argon is not supported by the server, falling back to plain login");
            m_knownArgonUrl.clear();
            this->tryAuth();
        } break;

        case ARGON_UNREACHABLE:
        case ARGON_INTERNAL_ERROR: {
            log::warn("Login failed: internal server error, argon is unreachable or failing");
            this->abortConnection("Internal server error (auth system is failing)");
        } break;

        default: {
            log::warn("Login failed: unknown reason {}", static_cast<int>(reason));
            this->abortConnection(fmt::format("Login failed due to unknown server error: {}", static_cast<int>(reason)));
        } break;
    }
}

static Result<> encodeAndSend(
    qn::Connection& conn,
    std::function<void(capnp::MallocMessageBuilder&)> func,
    bool reliable
) {
    capnp::MallocMessageBuilder msg;
    func(msg);

    size_t unpackedSize = capnp::computeSerializedSizeInWords(msg) * 8;
    qn::HeapByteWriter writer;
    writer.writeVarUint(unpackedSize).unwrap();
    auto unpSizeBuf = writer.written();

    kj::VectorOutputStream vos;
    vos.write(unpSizeBuf.data(), unpSizeBuf.size());
    capnp::writePackedMessage(vos, msg);

    auto data = std::vector<uint8_t>(vos.getArray().begin(), vos.getArray().end());

    conn.sendData(std::move(data), reliable);

    return Ok();
}

Result<> NetworkManagerImpl::sendToCentral(std::function<void(CentralMessage::Builder&)> func) {
    if (!m_centralConn.connected()) {
        return Err("Not connected to central server");
    }

    return encodeAndSend(m_centralConn, [&](capnp::MallocMessageBuilder& msg) {
        auto root = msg.initRoot<CentralMessage>();
        func(root);
    }, true);
}

Result<> NetworkManagerImpl::sendToGame(std::function<void(GameMessage::Builder&)> func, bool reliable) {
    if (!m_gameConn.connected()) {
        return Err("Not connected to game server");
    }

    return encodeAndSend(m_gameConn, [&](capnp::MallocMessageBuilder& msg) {
        auto root = msg.initRoot<GameMessage>();
        func(root);
    }, reliable);
}

std::optional<std::string> NetworkManagerImpl::getUToken() {
    return globed::value<std::string>(fmt::format("auth.last-utoken.{}", m_centralUrl));
}

void NetworkManagerImpl::setUToken(std::string token) {
    ValueManager::get().set(fmt::format("auth.last-utoken.{}", m_centralUrl), std::move(token));
}

void NetworkManagerImpl::clearUToken() {
    ValueManager::get().erase(fmt::format("auth.last-utoken.{}", m_centralUrl));
}

}
