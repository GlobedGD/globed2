#include "NetworkManagerImpl.hpp"
#include <globed/core/ValueManager.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/core/FriendListManager.hpp>
#include <globed/core/net/NetworkManager.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/data/PlayerDisplayData.hpp>
#include <globed/core/RoomManager.hpp>
#include <core/CoreImpl.hpp>
#include "data/helpers.hpp"

#include <argon/argon.hpp>
#include <qunet/Pinger.hpp>
#include <qunet/util/algo.hpp>
#include <qunet/util/hash.hpp>
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
        auto lock = m_connInfo.lock();

        // we assert that a game connection can only happen within the context of a central connection,
        // and this callback must never be invoked if we are not connected to a central server
        if (!lock->has_value()) {
            log::error("game connection state callback invoked without a central connection established!");
            return;
        }

        auto& connInfo = **lock;

        connInfo.m_gameConnState = state;
        if (state == qn::ConnectionState::Connected) {
            log::debug("connected to game server at {}", connInfo.m_gameServerUrl);
            connInfo.m_gameEstablished = true;

            // if there was a deferred join, try to login with session, otherwise just login
            if (connInfo.m_gsDeferredJoin) {
                this->sendGameLoginJoinRequest(*connInfo.m_gsDeferredJoin);
            } else {
                this->sendGameLoginRequest();
            }
        } else if (state == qn::ConnectionState::Disconnected) {
            log::debug("disconnected from game server at {}", connInfo.m_gameServerUrl);
            connInfo.m_gameEstablished = false;
            connInfo.m_gameServerUrl.clear();

            // if there was a deferred join, try to connect now
            if (connInfo.m_gsDeferredConnectJoin) {
                m_gameConn.setDebugOptions(getConnOpts());
                auto res = m_gameConn.connect(connInfo.m_gsDeferredConnectJoin->first);
                connInfo.m_gameServerUrl = connInfo.m_gsDeferredConnectJoin->first;

                if (!res) {
                    log::error("Failed to connect to game server {}: {}", connInfo.m_gsDeferredConnectJoin->first, res.unwrapErr().message());
                    connInfo.m_gameServerUrl.clear();
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
        switch (m_centralConnState.load()) {
            case qn::ConnectionState::Disconnected: {
                m_connInfo.lock()->reset();

                // wait for a connection..
                m_pendingConnectNotify.wait();
                auto url = *m_pendingConnectUrl.lock();

                auto connInfo = m_connInfo.lock();
                connInfo->emplace();
                m_centralUrl = std::string(url);

                m_centralConn.setDebugOptions(getConnOpts());
                m_disconnectRequested.store(false);

                auto res = m_centralConn.connect(url);
                if (!res) {
                    log::error("Failed to connect to central server at '{}': {}", url, res.unwrapErr().message());
                }
            } break;

            case qn::ConnectionState::DnsResolving:
            case qn::ConnectionState::Pinging:
            case qn::ConnectionState::Connecting:
            case qn::ConnectionState::Reconnecting:
            case qn::ConnectionState::Closing: {
                // do nothing!
                asp::time::sleep(Duration::fromMillis(10));
            } break;

            case qn::ConnectionState::Connected: {
                this->thrPingGameServers();
                this->thrMaybeResendOwnData();

                if (m_disconnectNotify.wait(Duration::fromMillis(10), [&] {
                    return m_disconnectRequested.load();
                })) {
                    // disconnect was requested, abort connection
                    log::debug("disconnect was requested, aborting connection");
                    m_disconnectRequested.store(false);

                    (void) m_centralConn.disconnect();
                    m_gameConn.cancelConnection();
                    (void) m_gameConn.disconnect();

                    m_finishedClosingNotify.wait(Duration::zero(), [&] {
                        return m_centralConnState.load() != qn::ConnectionState::Connected;
                    });

                    // if a game connection is still closing, wait
                    while (!m_gameConn.disconnected()) {
                        log::debug("waiting for game connection to terminate..");
                        asp::time::sleep(Duration::fromMillis(25));
                    }
                }
            } break;
        }
    });

    m_workerThread.start();

    argon::initConfigLock();
}

NetworkManagerImpl::~NetworkManagerImpl() {
    m_pendingConnectNotify.notifyAll();
    m_workerThread.stopAndWait();
}

NetworkManagerImpl& NetworkManagerImpl::get() {
    return *NetworkManager::get().m_impl;
}

void NetworkManagerImpl::thrPingGameServers() {
    auto connInfo = m_connInfo.lock();
    auto& servers = (*connInfo)->m_gameServers;

    for (auto& [srvkey, server] : servers) {
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
                auto connInfo = m_connInfo.lock();
                auto& servers = (*connInfo)->m_gameServers;
                auto it = servers.find(srvkey);
                if (it != servers.end()) {
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

void NetworkManagerImpl::thrMaybeResendOwnData() {
    auto& flm = FriendListManager::get();

    // check if icons or friend list need to be resent
    auto connInfo = m_connInfo.lock();
    bool friendList = !(**connInfo).m_sentFriendList && flm.isLoaded();
    bool icons = !(**connInfo).m_sentIcons;

    if (!icons && !friendList) {
        return;
    }

    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto update = msg.initUpdateOwnData();

        if (icons) {
            data::encodeIconData(gatherIconData(), update.initIcons());
        }

        if (friendList) {
            auto friends = flm.getFriends();

            auto fl = update.initFriendList(friends.size());
            size_t i = 0;
            for (int id : friends) {
                fl.set(i++, id);
            }
        }
    });

    (**connInfo).m_sentIcons = true;
    (**connInfo).m_sentFriendList = true;
}

Result<> NetworkManagerImpl::connectCentral(std::string_view url) {
    if (!m_centralConn.disconnected()) {
        return Err("Already connected to central server");
    }

    *m_pendingConnectUrl.lock() = std::string(url);
    m_pendingConnectNotify.notifyOne();

    FriendListManager::get().refresh();

    return Ok();
}

Result<> NetworkManagerImpl::disconnectCentral() {
    auto connInfo = m_connInfo.lock();

    if (!*connInfo) {
        return Err("Not connected to central server");
    }

    m_disconnectRequested.store(true);
    m_disconnectNotify.notifyOne();

    return Ok();
}

qn::ConnectionState NetworkManagerImpl::getConnState(bool game) {
    auto connInfo = m_connInfo.lock();

    if (game && *connInfo) {
        return (*connInfo)->m_gameConnState.load();
    } else if (game) {
        return qn::ConnectionState::Disconnected;
    } else {
        return m_centralConnState.load();
    }
}

std::optional<uint8_t> NetworkManagerImpl::getPreferredServer() {
    auto lock = m_connInfo.lock();
    if (!*lock) {
        return std::nullopt;
    }

    auto& connInfo = **lock;

    auto& servers = connInfo.m_gameServers;

    if (auto value = globed::value<std::string>("core.net.preferred-server")) {
        for (auto& srv : servers) {
            if (srv.second.stringId == *value) {
                return srv.second.id;
            }
        }
    }

    // fallback to the server with the lowest latency
    if (!servers.empty()) {
        auto it = std::min_element(servers.begin(), servers.end(),
            [](const auto& a, const auto& b) {
                return a.second.lastLatency < b.second.lastLatency;
            });
        return it->second.id;
    }

    return std::nullopt;
}

std::vector<GameServer> NetworkManagerImpl::getGameServers() {
    auto lock = m_connInfo.lock();
    if (!*lock) {
        return {};
    }

    std::vector<GameServer> out;

    for (auto& server : (**lock).m_gameServers) {
        out.push_back(server.second);
    }

    return out;
}

bool NetworkManagerImpl::isConnected() const {
    return (**m_connInfo.lock()).m_established;
}

Duration NetworkManagerImpl::getGamePing() {
    return m_gameConn.getLatency();
}

Duration NetworkManagerImpl::getCentralPing() {
    return m_centralConn.getLatency();
}

std::string NetworkManagerImpl::getCentralIdent() {
    if (m_centralUrl.empty()) return "";

    auto hash = qn::blake3Hash(m_centralUrl).toString();
    hash.resize(16);
    return hash;
}

std::string NetworkManagerImpl::getStoredModPassword() {
    if (!globed::setting<bool>("core.mod.remember-password")) {
        return "";
    }

    auto pwkey = fmt::format("core.mod.last-password-{}", this->getCentralIdent());

    return globed::value<std::string>(pwkey).value_or({});
}

void NetworkManagerImpl::storeModPassword(const std::string& pw) {
    auto pwkey = fmt::format("core.mod.last-password-{}", this->getCentralIdent());
    if (globed::setting<bool>("core.mod.remember-password")) {
        globed::setValue(pwkey, pw);
    } else {
        globed::setValue(pwkey, ""); // explicitly clear for security
    }
}

uint32_t NetworkManagerImpl::getGameTickrate() {
    auto lock = m_connInfo.lock();
    return *lock ? (*lock)->m_gameTickrate : 0;
}

std::vector<UserRole> NetworkManagerImpl::getAllRoles() {
    auto lock = m_connInfo.lock();
    return *lock ? (*lock)->m_allRoles : std::vector<UserRole>{};
}

std::vector<UserRole> NetworkManagerImpl::getUserRoles() {
    auto lock = m_connInfo.lock();
    return *lock ? (*lock)->m_userRoles : std::vector<UserRole>{};
}

std::optional<UserRole> NetworkManagerImpl::getUserHighestRole() {
    auto lock = m_connInfo.lock();
    if (!*lock) {
        return std::nullopt;
    }

    // we assume that roles are sorted by priority, so the first one is the highest
    auto& roles = (**lock).m_userRoles;
    return roles.empty() ? std::nullopt : std::make_optional(roles.front());
}

std::optional<UserRole> NetworkManagerImpl::findRole(uint8_t roleId) {
    auto lock = m_connInfo.lock();
    if (!*lock) {
        return std::nullopt;
    }

    for (auto& role : (**lock).m_allRoles) {
        if (role.id == roleId) {
            return role;
        }
    }

    return std::nullopt;
}

std::optional<UserRole> NetworkManagerImpl::findRole(std::string_view roleId) {
    auto lock = m_connInfo.lock();
    if (!*lock) {
        return std::nullopt;
    }

    for (auto& role : (**lock).m_allRoles) {
        if (role.stringId == roleId) {
            return role;
        }
    }

    return std::nullopt;
}

bool NetworkManagerImpl::isModerator() {
    auto lock = m_connInfo.lock();
    return *lock ? (*lock)->m_isModerator : false;
}

bool NetworkManagerImpl::isAuthorizedModerator() {
    auto lock = m_connInfo.lock();
    return *lock ? (*lock)->m_isAuthorizedModerator : false;
}

void NetworkManagerImpl::invalidateIcons() {
    auto lock = m_connInfo.lock();
    if (*lock) {
        (**lock).m_sentIcons = false;
    }
}

void NetworkManagerImpl::invalidateFriendList() {
    auto lock = m_connInfo.lock();
    if (*lock) {
        (**lock).m_sentFriendList = false;
    }
}

void NetworkManagerImpl::markAuthorizedModerator() {
    auto lock = m_connInfo.lock();
    if (*lock) {
        (**lock).m_isAuthorizedModerator = true;
    }
}

void NetworkManagerImpl::onCentralConnected() {
    log::debug("connection to central server established, trying to log in");

    this->tryAuth();
}

void NetworkManagerImpl::tryAuth() {
    auto gam = GJAccountManager::get();
    int accountId = gam->m_accountID;
    int userId = GameManager::get()->m_playerUserID;

    auto lock = m_connInfo.lock();
    if (!*lock) {
        log::error("Cannot authenticate, not connected to central server");
        return;
    }

    auto& connInfo = **lock;

    if (auto stoken = this->getUToken()) {
        (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
            log::debug("attempting login with user token {}", *stoken);
            auto loginUToken = msg.initLoginUToken();
            loginUToken.setToken(*stoken);
            loginUToken.setAccountId(accountId);
            data::encodeIconData(gatherIconData(), loginUToken.initIcons());
        });
    } else if (!connInfo.m_knownArgonUrl.empty()) {
        (void) argon::setServerUrl(connInfo.m_knownArgonUrl);
        connInfo.m_waitingForArgon = true;
        lock.unlock();

        auto res = argon::startAuth([&](Result<std::string> res) {
            (**m_connInfo.lock()).m_waitingForArgon = false;

            if (!res) {
                this->abortConnection(fmt::format("failed to complete Argon auth: {}", res.unwrapErr()));
                return;
            }

            this->doArgonAuth(std::move(*res));
        });

        if (!res) {
            (**m_connInfo.lock()).m_waitingForArgon = false;
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
            data::encodeIconData(gatherIconData(), loginPlain.initIcons());
        });
    }
}

void NetworkManagerImpl::doArgonAuth(std::string token) {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        log::debug("attempting login with argon token");
        auto loginArgon = msg.initLoginArgon();
        loginArgon.setToken(token);
        loginArgon.setAccountId(GJAccountManager::get()->m_accountID);
        data::encodeIconData(gatherIconData(), loginArgon.initIcons());
    });
}

void NetworkManagerImpl::abortConnection(std::string reason) {
    log::warn("aborting connection to central server: {}", reason);
    (void) m_centralConn.disconnect();
}

void NetworkManagerImpl::onCentralDisconnected() {
    log::debug("connection to central server lost!");

    Loader::get()->queueInMainThread([this] {
        CoreImpl::get().onServerDisconnected();
    });

    m_finishedClosingNotify.notifyAll();

    // in case this is an abnormal closure, also notify the disconnect notify
    m_disconnectRequested.store(true);
    m_disconnectNotify.notifyAll();
}

void NetworkManagerImpl::sendJoinSession(SessionId id) {
    auto lock = m_connInfo.lock();
    if (!*lock) {
        log::error("Cannot join session, not connected to central server");
        return;
    }

    auto& connInfo = **lock;

    log::debug("Joining session with ID {}", id.asU64());

    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto joinSession = msg.initJoinSession();
        joinSession.setSessionId(id);
    });

    // find the game server
    uint8_t serverId = id.serverId();
    for (auto& srv : connInfo.m_gameServers) {
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
    auto lock = m_connInfo.lock();
    if (!*lock) {
        log::error("Trying to join a session while not connected to a central server!");
        return;
    }

    auto& connInfo = **lock;

    if (m_gameConn.connected()) {
        if (connInfo.m_gameServerUrl != serverUrl) {
            connInfo.m_gsDeferredConnectJoin = std::make_pair(std::string(serverUrl), id);
            (void) m_gameConn.disconnect();
        } else if (connInfo.m_gameEstablished) {
            // same server, just send the join request
            this->sendGameJoinRequest(id);
        } else {
            this->sendGameLoginJoinRequest(id);
        }
    } else {
        // not connected, connect to the game server and join later
        connInfo.m_gsDeferredJoin = id;
        connInfo.m_gameServerUrl = std::string(serverUrl);

        (void) m_gameConn.cancelConnection();
        (void) m_gameConn.disconnect();

        m_gameConn.setDebugOptions(getConnOpts());
        auto res = m_gameConn.connect(serverUrl);
        if (!res) {
            log::error("Failed to connect to {}: {}", serverUrl, res.unwrapErr().message());
            connInfo.m_gameServerUrl.clear();
            connInfo.m_gsDeferredJoin.reset();
        }
    }
}

void NetworkManagerImpl::sendGameLoginJoinRequest(SessionId id) {
    (void) this->sendToGame([&](GameMessage::Builder& msg) {
        auto loginJoin = msg.initLoginUTokenAndJoin();
        loginJoin.setAccountId(GJAccountManager::get()->m_accountID);
        loginJoin.setToken(this->getUToken().value_or(""));
        loginJoin.setSessionId(id);
        loginJoin.setPasscode(RoomManager::get().getPasscode());
        data::encodeIconData(gatherIconData(), loginJoin.initIcons());
    });
}

void NetworkManagerImpl::sendGameLoginRequest() {
    (void) this->sendToGame([&](GameMessage::Builder& msg) {
        auto login = msg.initLoginUToken();
        login.setAccountId(GJAccountManager::get()->m_accountID);
        login.setToken(this->getUToken().value_or(""));
        data::encodeIconData(gatherIconData(), login.initIcons());
    });
}

void NetworkManagerImpl::sendGameJoinRequest(SessionId id) {
    (void) this->sendToGame([&](GameMessage::Builder& msg) {
        auto join = msg.initJoinSession();
        join.setSessionId(id);
        join.setPasscode(RoomManager::get().getPasscode());
    });
}

void NetworkManagerImpl::sendPlayerState(const PlayerState& state, const std::vector<int>& dataRequests, CCPoint cameraCenter, float cameraRadius) {
    auto lock = m_connInfo.lock();
    if (!*lock || !(**lock).m_gameEstablished) {
        log::warn("Cannot send player state, not connected to game server");
        return;
    }

    auto& connInfo = **lock;

    (void) this->sendToGame([&](GameMessage::Builder& msg) {
        auto playerData = msg.initPlayerData();
        auto data = playerData.initData();
        data::encodePlayerState(state, data);

        playerData.setCameraX(cameraCenter.x);
        playerData.setCameraY(cameraCenter.y);
        playerData.setCameraRadius(cameraRadius);

        auto reqs = playerData.initDataRequests(dataRequests.size());
        for (size_t i = 0; i < dataRequests.size(); ++i) {
            reqs.set(i, dataRequests[i]);
        }

        auto evs = playerData.initEvents(std::min<size_t>(connInfo.m_gameEventQueue.size(), 64));
        for (size_t i = 0; i < evs.size(); i++) {
            auto& event = connInfo.m_gameEventQueue.front();

            auto ev = evs[i];
            ev.setType(event.type);
            ev.setData({(kj::byte*) event.data.data(), event.data.size()});

            connInfo.m_gameEventQueue.pop();
        }
    }, !connInfo.m_gameEventQueue.empty());
}

void NetworkManagerImpl::sendLevelScript(const std::vector<EmbeddedScript>& scripts) {
    (void) this->sendToGame([&](GameMessage::Builder& msg) {
        data::encodeSendLevelScriptMessage(scripts, msg);
    });
}

void NetworkManagerImpl::queueGameEvent(Event&& event) {
    auto lock = m_connInfo.lock();
    if (!*lock) {
        return;
    }

    (**lock).m_gameEventQueue.push(std::move(event));
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

        data::encodeRoomSettings(settings, createRoom.initSettings());
    });
}

void NetworkManagerImpl::sendJoinRoom(uint32_t id, uint32_t passcode) {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto joinRoom = msg.initJoinRoom();
        joinRoom.setRoomId(id);
        joinRoom.setPasscode(passcode);
        RoomManager::get().setAttemptedPasscode(passcode);
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

void NetworkManagerImpl::sendAssignTeam(int accountId, uint16_t teamId) {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto assign = msg.initAssignTeam();
        assign.setAccountId(accountId);
        assign.setTeamId(teamId);
    });
}

void NetworkManagerImpl::sendCreateTeam(cocos2d::ccColor4B color) {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto create = msg.initCreateTeam();
        create.setColor(data::encodeColor4(color));
    });
}

void NetworkManagerImpl::sendDeleteTeam(uint16_t teamId) {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto del = msg.initDeleteTeam();
        del.setTeamId(teamId);
    });
}

void NetworkManagerImpl::sendUpdateTeam(uint16_t teamId, cocos2d::ccColor4B color) {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto update = msg.initUpdateTeam();
        update.setTeamId(teamId);
        update.setColor(data::encodeColor4(color));
    });
}

void NetworkManagerImpl::sendGetTeamMembers() {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        msg.initGetTeamMembers();
    });
}

void NetworkManagerImpl::sendAdminNotice(const std::string& message, const std::string& user, int roomId, int levelId, bool canReply) {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto adminNotice = msg.initAdminNotice();
        adminNotice.setMessage(message);
        adminNotice.setTargetUser(user);
        adminNotice.setRoomId(roomId);
        adminNotice.setLevelId(levelId);
        adminNotice.setCanReply(canReply);
        adminNotice.setShowSender(false); // TODO: showSender
    });
}

void NetworkManagerImpl::sendAdminNoticeEveryone(const std::string& message) {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto adminNotice = msg.initAdminNoticeEveryone();
        adminNotice.setMessage(message);
    });
}

void NetworkManagerImpl::sendAdminLogin(const std::string& password) {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto adminLogin = msg.initAdminLogin();
        adminLogin.setPassword(password);
    });
}

void NetworkManagerImpl::sendAdminFetchUser(const std::string& query) {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto fetchUser = msg.initAdminFetchUser();
        fetchUser.setQuery(query);
    });
}

void NetworkManagerImpl::sendAdminFetchMods() {
    (void) this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto fetchUser = msg.initAdminFetchMods();
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
            auto lock = m_connInfo.lock();
            auto& connInfo = **lock;

            auto loginOk = msg.getLoginOk();

            if (loginOk.hasServers()) {
                auto servers = loginOk.getServers();
                updateServers(connInfo.m_gameServers, servers);
            }

            auto msg = data::decodeCentralLoginOk(loginOk);
            connInfo.m_allRoles = msg.allRoles;
            connInfo.m_userRoles = msg.userRoles;
            connInfo.m_established = true;
            connInfo.m_isModerator = msg.isModerator;

            if (!msg.newToken.empty()) {
                this->setUToken(msg.newToken);
            }

            this->invokeListeners(msg);
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

            (**m_connInfo.lock()).m_knownArgonUrl = loginRequired.getArgonUrl();
            this->clearUToken();
            this->tryAuth();
        } break;

        case CentralMessage::BANNED: {
            // TODO
        } break;

        case CentralMessage::SERVERS_CHANGED: {
            auto lock = m_connInfo.lock();
            auto& connInfo = **lock;

            auto serversChanged = msg.getServersChanged();
            auto servers = serversChanged.getServers();

            updateServers(connInfo.m_gameServers, servers);
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

        case CentralMessage::ROOM_BANNED: {
            // TODO
        } break;

        case CentralMessage::ROOM_LIST: {
            auto roomList = msg.getRoomList();

            if (auto msg = data::decodeRoomListMessage(roomList)) {
                this->invokeListeners(*msg);
            } else {
                log::warn("Received invalid room list message");
            }
        } break;

        case CentralMessage::TEAM_CREATION_RESULT: {
            auto result = msg.getTeamCreationResult();

            this->invokeListeners(data::decodeTeamCreationResultMessage(result));
        } break;

        case CentralMessage::TEAM_CHANGED: {
            auto result = msg.getTeamChanged();

            this->invokeListeners(data::decodeTeamChangedMessage(result));
        } break;

        case CentralMessage::TEAM_MEMBERS: {
            auto result = msg.getTeamMembers();

            this->invokeListeners(data::decodeTeamMembersMessage(result));
        } break;

        case CentralMessage::TEAMS_UPDATED: {
            auto result = msg.getTeamsUpdated();

            this->invokeListeners(data::decodeTeamsUpdatedMessage(result));
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

        case CentralMessage::NOTICE: {
            auto notice = msg.getNotice();

            this->invokeListeners(msg::NoticeMessage {
                .senderId = notice.getSenderId(),
                .senderName = notice.getSenderName(),
                .message = notice.getMessage(),
                .canReply = notice.getCanReply(),
            });
        } break;

        case CentralMessage::WARN: {
            // TODO
        } break;

        case CentralMessage::ADMIN_RESULT: {
            auto adminResult = msg.getAdminResult();

            // this is a bit hacky, but if the result is ok, mark the client as authorized admin
            // a successful result can only be sent as a response to auth or to an actual admin packet
            if (adminResult.getSuccess()) {
                this->markAuthorizedModerator();
            }

            this->invokeListeners(msg::AdminResultMessage {
                .success = adminResult.getSuccess(),
                .error = adminResult.getError(),
            });
        } break;

        case CentralMessage::ADMIN_FETCH_RESPONSE: {
            auto result = msg.getAdminFetchResponse();

            this->invokeListeners(data::decodeAdminFetchResponseMessage(result));
        } break;

        case CentralMessage::ADMIN_FETCH_MODS_RESPONSE: {
            auto result = msg.getAdminFetchModsResponse();

            this->invokeListeners(data::decodeFetchModsResponseMessage(result));
        } break;

        case CentralMessage::ADMIN_LOGS_RESPONSE: {
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
            auto lock = m_connInfo.lock();
            auto& connInfo = **lock;

            auto loginOk = msg.getLoginOk();

            connInfo.m_gameEstablished = true;
            connInfo.m_gameTickrate = loginOk.getTickrate();
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
            (**m_connInfo.lock()).m_knownArgonUrl.clear();
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
    return globed::value<std::string>(fmt::format("auth.last-utoken.{}", this->getCentralIdent()));
}

void NetworkManagerImpl::setUToken(std::string token) {
    ValueManager::get().set(fmt::format("auth.last-utoken.{}", this->getCentralIdent()), std::move(token));
}

void NetworkManagerImpl::clearUToken() {
    ValueManager::get().erase(fmt::format("auth.last-utoken.{}", this->getCentralIdent()));
}

}
