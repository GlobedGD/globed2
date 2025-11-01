#include "NetworkManagerImpl.hpp"
#include <globed/core/ValueManager.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/core/FriendListManager.hpp>
#include <globed/core/net/NetworkManager.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/data/PlayerDisplayData.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/PopupManager.hpp>
#include <globed/util/gd.hpp>
#include <core/CoreImpl.hpp>
#include "data/helpers.hpp"
#include <bb_public.hpp>

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

static void gatherUserSettings(auto&& out) {
    out.setHideInLevel(globed::setting<bool>("core.user.hide-in-levels"));
    out.setHideInMenus(globed::setting<bool>("core.user.hide-in-menus"));
    out.setHideRoles(globed::setting<bool>("core.user.hide-roles"));
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

static argon::AccountData g_argonData{};

namespace globed {

void GameServer::updateLatency(uint32_t latency) {
    if (avgLatency == -1) {
        avgLatency = latency;
    } else {
        avgLatency = qn::exponentialMovingAverage(avgLatency, latency, 0.4);
    }

    lastLatency = latency;
}

std::string_view connectionStateToStr(qn::ConnectionState state) {
    using enum qn::ConnectionState;
    switch (state) {
        case Disconnected: return "Disconnected";
        case DnsResolving: return "DnsResolving";
        case Pinging: return "Pinging";
        case Connecting: return "Connecting";
        case Connected: return "Connected";
        case Closing: return "Closing";
        case Reconnecting: return "Reconnecting";
        default: return "unknown";
    }
}

NetworkManagerImpl::NetworkManagerImpl() {
    m_hasSecure = bb_init();

    // TODO (low): measure how much of an impact those have on bandwidth
    m_centralConn.setActiveKeepaliveInterval(Duration::fromSecs(45));
    m_gameConn.setActiveKeepaliveInterval(Duration::fromSecs(10));

    m_centralConn.setConnectionStateCallback([this](qn::ConnectionState state) {
        m_centralConnState = state;

        log::info("Central connection state: {}", connectionStateToStr(state));

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
            log::error("failed to process message from central server: {}", *err);
        }
    });

    m_gameConn.setConnectionStateCallback([this](qn::ConnectionState state) {
        log::info("Game connection state: {}", connectionStateToStr(state));

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
                auto& join = *connInfo.m_gsDeferredJoin;
                this->sendGameLoginRequest(join.id, join.platformer);
            } else {
                this->sendGameLoginRequest();
            }

            // if there was a queued script message, send it
            auto queuedScripts = std::move(connInfo.m_queuedScripts);
            if (!queuedScripts.empty()) {
                this->sendLevelScript(queuedScripts);
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
                    FunctionQueue::get().queue([url, err = std::move(res).unwrapErr()] mutable {
                        log::error("Failed to connect to central server at '{}': {}", url, err.message());
                        globed::alertFormat(
                            "Globed Error",
                            "Failed to connect to central server at <cp>'{}'</c>: <cy>{}</c>",
                            url, err.message()
                        );
                    });
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
                auto lock = m_connInfo.lock();
                if (*lock) {
                    auto& info = **lock;
                    this->thrPingGameServers(info);
                    this->thrMaybeResendOwnData(info);

                    // this is a mess
                    bool timedOut = info.m_authenticating && info.m_triedAuthAt.elapsed() > Duration::fromSecs(5);

                    if (timedOut && !info.m_waitingForArgon && !info.m_established) {
                        // cancel connection
                        lock.unlock();
                        this->abortConnection("Authentication timed out");
                        return;
                    }
                }

                lock.unlock();

                if (m_disconnectNotify.wait(Duration::fromMillis(10), [&] {
                    return m_disconnectRequested.load();
                })) {
                    // disconnect was requested, abort connection
                    log::debug("disconnect was requested, aborting connection");
                    m_disconnectRequested.store(false);

                    m_manualDisconnect = true;

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

                    log::debug("Connection abort finished");
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

    m_destructing = true;
}

NetworkManagerImpl& NetworkManagerImpl::get() {
    return *NetworkManager::get().m_impl;
}

void NetworkManagerImpl::thrPingGameServers(ConnectionInfo& connInfo) {
    auto& servers = connInfo.m_gameServers;

    for (auto& [srvkey, server] : servers) {
        if (server.lastPingTime.elapsed() > getPingInterval(server.sentPings)) {
            server.sentPings++;
            server.lastPingTime = Instant::now();

            // send a ping to the server
            auto res = qn::Pinger::get().pingUrl(server.url, [this, srvkey = srvkey](qn::PingResult res) {
                if (res.timedOut) {
                    log::debug("Ping to server {} timed out!", srvkey);
                    return;
                }

                // update the server latency
                auto connInfo = m_connInfo.lock();
                if (!*connInfo) return;

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

void NetworkManagerImpl::thrMaybeResendOwnData(ConnectionInfo& connInfo) {
    auto& flm = FriendListManager::get();

    // check if icons or friend list need to be resent
    bool friendList = !connInfo.m_sentFriendList && flm.isLoaded();
    bool icons = !connInfo.m_sentIcons;

    if (!icons && !friendList) {
        return;
    }

    // don't send if we haven't authorized yet
    if (!connInfo.m_established) {
        return;
    }

    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto update = msg.initUpdateOwnData();

        if (icons) {
            data::encode(gatherIconData(), update.initIcons());
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

    connInfo.m_sentIcons = true;
    connInfo.m_sentFriendList = true;
}

Result<> NetworkManagerImpl::connectCentral(std::string_view url) {
    if (!m_centralConn.disconnected()) {
        return Err("Already connected to central server");
    }

    g_argonData = argon::getGameAccountData();

    *m_pendingConnectUrl.lock() = std::string(url);
    m_pendingConnectNotify.notifyOne();
    m_manualDisconnect = false;
    m_abortCause.lock()->first.clear();

    FriendListManager::get().refresh();

    return Ok();
}

Result<> NetworkManagerImpl::disconnectCentral() {
    auto connInfo = m_connInfo.lock();

    if (!*connInfo) {
        return Err("Not connected to central server");
    }

    this->disconnectInner();

    return Ok();
}

Result<> NetworkManagerImpl::cancelConnection() {
    m_centralConn.cancelConnection();
    this->disconnectInner();
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
    auto lock = m_connInfo.lock();
    return *lock && (*lock)->m_established;
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

    return globed::value<std::string>(pwkey).value_or(std::string{});
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

std::vector<uint8_t> NetworkManagerImpl::getUserRoleIds() {
    auto lock = m_connInfo.lock();
    return *lock ? (*lock)->m_userRoleIds : std::vector<uint8_t>{};
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
    return this->getModPermissions().isModerator;
}

bool NetworkManagerImpl::isAuthorizedModerator() {
    auto lock = m_connInfo.lock();
    return *lock ? (*lock)->m_authorizedModerator : false;
}

ModPermissions NetworkManagerImpl::getModPermissions() {
    auto lock = m_connInfo.lock();
    return *lock ? (*lock)->m_perms : ModPermissions{};
}

PunishReasons NetworkManagerImpl::getModPunishReasons() {
    auto lock = m_connInfo.lock();
    return *lock ? (*lock)->m_punishReasons : PunishReasons{};
}

std::optional<SpecialUserData> NetworkManagerImpl::getOwnSpecialData() {
    auto lock = m_connInfo.lock();
    if (*lock && ((**lock).m_userRoleIds.size() || (**lock).m_nameColor)) {
        SpecialUserData data{};
        data.roleIds = (**lock).m_userRoleIds;
        data.nameColor = (**lock).m_nameColor;
        return data;
    }

    return std::nullopt;
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
        (**lock).m_authorizedModerator = true;
    }
}

std::optional<FeaturedLevelMeta> NetworkManagerImpl::getFeaturedLevel() {
    auto lock = m_connInfo.lock();
    return *lock ? (**lock).m_featuredLevel : std::nullopt;
}

bool NetworkManagerImpl::hasViewedFeaturedLevel() {
    return this->getLastFeaturedLevelId() == this->getFeaturedLevel().value_or(FeaturedLevelMeta{}).levelId;
}

void NetworkManagerImpl::setViewedFeaturedLevel() {
    this->setLastFeaturedLevelId(this->getFeaturedLevel().value_or(FeaturedLevelMeta{}).levelId);
}

void NetworkManagerImpl::onCentralConnected() {
    globed::setValue<bool>("core.was-connected", true);

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
        connInfo.startedAuth();
        this->sendCentralAuth(AuthKind::Utoken, *stoken);
    } else if (!connInfo.m_knownArgonUrl.empty()) {
        (void) argon::setServerUrl(connInfo.m_knownArgonUrl);
        connInfo.m_waitingForArgon = true;
        lock.unlock();

        auto res = argon::startAuthWithAccount(g_argonData, [this](Result<std::string> res) {
            auto lock = m_connInfo.lock();
            (**lock).m_waitingForArgon = false;
            (**lock).startedAuth();
            lock.unlock();

            if (!res) {
                this->abortConnection(fmt::format("failed to complete Argon auth: {}", res.unwrapErr()));
                return;
            }

            this->sendCentralAuth(AuthKind::Argon, *res);
        });

        if (!res) {
            (**m_connInfo.lock()).m_waitingForArgon = false;
            this->abortConnection(fmt::format("failed to start Argon auth: {}", res.unwrapErr()));
            return;
        }
    } else {
        connInfo.startedAuth();
        this->sendCentralAuth(AuthKind::Plain);
    }
}

void NetworkManagerImpl::sendCentralAuth(AuthKind kind, const std::string& token) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        int accountId = GJAccountManager::get()->m_accountID;
        int userId = GameManager::get()->m_playerUserID;

        auto login = msg.initLogin();
        login.setAccountId(accountId);
        data::encode(gatherIconData(), login.initIcons());
        if (kind != AuthKind::Plain) {
            auto uid = this->computeUident(accountId);
            login.setUident(kj::arrayPtr(uid.data(), uid.size()));
        }
        gatherUserSettings(login.initSettings());

        switch (kind) {
            case AuthKind::Utoken: {
                log::debug("attempting login with user token {}", token);
                login.setUtoken(token);
            } break;

            case AuthKind::Argon: {
                log::debug("attempting login with argon token ({})", accountId);
                login.setArgon(token);
            } break;

            case AuthKind::Plain: {
                log::debug("attempting plain login");
                auto plain = login.initPlain();
                plain.setAccountId(accountId);
                plain.setUserId(userId);
                plain.setUsername(GJAccountManager::get()->m_username);
            } break;
        }
    });
}

void NetworkManagerImpl::abortConnection(std::string reason, bool silent) {
    log::warn("aborting connection to central server: {}", reason);
    *m_abortCause.lock() = std::make_pair(std::move(reason), silent);
    (void) m_centralConn.disconnect();
}

void NetworkManagerImpl::onCentralDisconnected() {
    if (!m_destructing) {
        globed::setValue<bool>("core.was-connected", false);
    }

    bool showPopup = !m_manualDisconnect;
    std::string message = "client initiated disconnect";

    if (!m_manualDisconnect) {
        auto abortCause = m_abortCause.lock();

        if (!abortCause->first.empty()) {
            message = std::move(abortCause->first);
        } else {
            auto err = m_centralConn.lastError();
            if (err == qn::ConnectionError::Success) {
                message = "Connection cancelled";
            } else {
                message = err.message();
            }
        }

        // if this was a silent abort, don't show a popup
        if (abortCause->second) {
            showPopup = false;
        }
    }

    log::debug("connection to central server lost: {}", message);

    FunctionQueue::get().queue([this, showPopup, message = std::move(message)] {
        CoreImpl::get().onServerDisconnected();

        if (showPopup) {
            auto alert = PopupManager::get().alertFormat("Globed Error", "Connection lost: <cy>{}</c>", message);
            alert.showQueue();
        }
    });

    m_finishedClosingNotify.notifyAll();

    // in case this is an abnormal closure, also notify the disconnect notify
    m_disconnectRequested.store(true);
    m_disconnectNotify.notifyAll();
}

void NetworkManagerImpl::sendJoinSession(SessionId id, bool platformer) {
    auto lock = m_connInfo.lock();
    if (!*lock) {
        log::error("Cannot join session, not connected to central server");
        return;
    }

    auto& connInfo = **lock;

    log::debug("Joining session with ID {}", id.asU64());

    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto joinSession = msg.initJoinSession();
        joinSession.setSessionId(id);
    });

    // find the game server
    uint8_t serverId = id.serverId();
    for (auto& srv : connInfo.m_gameServers) {
        if (srv.second.id == serverId) {
            this->joinSessionWith(srv.second.url, id, platformer);
            return;
        }
    }

    // not found!
    log::error("Tried joining an invalid session, game server with ID {} does not exist", static_cast<int>(serverId));
}

void NetworkManagerImpl::sendLeaveSession() {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        msg.initLeaveSession();
    });

    this->sendToGame([&](GameMessage::Builder& msg) {
        msg.initLeaveSession();
    });
}

void NetworkManagerImpl::joinSessionWith(std::string_view serverUrl, SessionId id, bool platformer) {
    // if already connected to another game server, disconnect and wait for the connection to close
    auto lock = m_connInfo.lock();
    if (!*lock) {
        log::error("Trying to join a session while not connected to a central server!");
        return;
    }

    auto& connInfo = **lock;

    if (m_gameConn.connected()) {
        if (connInfo.m_gameServerUrl != serverUrl) {
            connInfo.m_gsDeferredConnectJoin = std::make_pair(
                std::string(serverUrl),
                DeferredSessionJoin { id, platformer }
            );
            (void) m_gameConn.disconnect();
        } else if (connInfo.m_gameEstablished) {
            // same server, just send the join request
            this->sendGameJoinRequest(id, platformer);
        } else {
            this->sendGameLoginRequest(id, platformer);
        }
    } else {
        // not connected, connect to the game server and join later
        connInfo.m_gsDeferredJoin = DeferredSessionJoin { id, platformer };
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

void NetworkManagerImpl::sendGameLoginRequest(SessionId id, bool platformer) {
    this->sendToGame([&](GameMessage::Builder& msg) {
        auto login = msg.initLogin();
        login.setAccountId(GJAccountManager::get()->m_accountID);
        login.setToken(this->getUToken().value_or(""));
        data::encode(gatherIconData(), login.initIcons());
        gatherUserSettings(login.initSettings());

        if (id.asU64() != 0) {
            login.setSessionId(id);
            login.setPasscode(RoomManager::get().getPasscode());
            login.setPlatformer(platformer);
        }
    });
}

void NetworkManagerImpl::sendGameJoinRequest(SessionId id, bool platformer) {
    this->sendToGame([&](GameMessage::Builder& msg) {
        auto join = msg.initJoinSession();
        join.setSessionId(id);
        join.setPlatformer(platformer);
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

    this->sendToGame([&](GameMessage::Builder& msg) {
        auto playerData = msg.initPlayerData();
        auto data = playerData.initData();
        data::encode(state, data);

        playerData.setCameraX(cameraCenter.x);
        playerData.setCameraY(cameraCenter.y);
        playerData.setCameraRadius(cameraRadius);

        auto reqs = playerData.initDataRequests(dataRequests.size());
        for (size_t i = 0; i < dataRequests.size(); ++i) {
            reqs.set(i, dataRequests[i]);
        }

        qn::HeapByteWriter eventEncoder;
        for (size_t i = 0; i < std::min<size_t>(64, connInfo.m_gameEventQueue.size()); i++) {
            auto& event = connInfo.m_gameEventQueue.front();
            if (auto err = event.encode(eventEncoder).err()) {
                log::warn("Failed to encode event: {}", *err);
            }

            connInfo.m_gameEventQueue.pop();
        }

        auto eventData = std::move(eventEncoder).intoVector();
        playerData.setEventData(kj::ArrayPtr{eventData.data(), eventData.size()});
    }, !connInfo.m_gameEventQueue.empty());
}

void NetworkManagerImpl::queueLevelScript(const std::vector<EmbeddedScript>& scripts) {
    auto lock = m_connInfo.lock();
    if (!*lock) return;

    auto& connInfo = **lock;
    if (connInfo.m_gameEstablished) {
        (*lock)->m_queuedScripts.clear();
        this->sendLevelScript(scripts);
    } else {
        (*lock)->m_queuedScripts = scripts;
    }
}

void NetworkManagerImpl::sendLevelScript(const std::vector<EmbeddedScript>& scripts) {
    this->sendToGame([&](GameMessage::Builder& msg) {
        data::encode(scripts, msg.initSendLevelScript());
    }, true, true);
}

void NetworkManagerImpl::queueGameEvent(OutEvent&& event) {
    auto lock = m_connInfo.lock();
    if (!*lock) {
        return;
    }

    (**lock).m_gameEventQueue.push(std::move(event));
}

void NetworkManagerImpl::sendVoiceData(const EncodedAudioFrame& frame) {
    this->sendToGame([&](GameMessage::Builder& msg) {
        auto voice = msg.initVoiceData();
        auto frames = voice.initFrames(frame.size());

        for (size_t i = 0; i < frame.size(); ++i) {
            auto& fr = frame.getFrames()[i];
            frames.set(i, kj::arrayPtr(fr.data.get(), fr.size));
        }
    }, false, true);
}

void NetworkManagerImpl::sendQuickChat(uint32_t id) {
    this->sendToGame([&](GameMessage::Builder& msg) {
        auto qc = msg.initQuickChat();
        qc.setId(id);
    }, false);
}

void NetworkManagerImpl::sendUpdateUserSettings() {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto update = msg.initUpdateUserSettings();
        gatherUserSettings(update.initSettings());
    });

    this->sendToGame([&](GameMessage::Builder& msg) {
        auto update = msg.initUpdateUserSettings();
        gatherUserSettings(update.initSettings());
    });
}

void NetworkManagerImpl::sendRoomStateCheck() {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        msg.setCheckRoomState();
    });
}

void NetworkManagerImpl::sendRequestRoomPlayers(const std::string& nameFilter) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto reqr = msg.initRequestRoomPlayers();
        reqr.setNameFilter(nameFilter);
    });
}

void NetworkManagerImpl::sendRequestGlobalPlayerList(const std::string& nameFilter) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto reqr = msg.initRequestGlobalPlayerList();
        reqr.setNameFilter(nameFilter);
    });
}

void NetworkManagerImpl::sendRequestLevelList() {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        msg.initRequestLevelList();
    });
}

void NetworkManagerImpl::sendRequestPlayerCounts(const std::vector<uint64_t>& sessions) {
    return this->sendRequestPlayerCounts(std::span{sessions.data(), sessions.size()});
}

void NetworkManagerImpl::sendRequestPlayerCounts(std::span<const uint64_t> sessions) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto reqr = msg.initRequestPlayerCounts();
        reqr.setLevels(kj::arrayPtr(sessions.data(), sessions.size()));
    });
}

void NetworkManagerImpl::sendRequestPlayerCounts(std::span<const SessionId> sessions) {
    return this->sendRequestPlayerCounts(std::span{(const uint64_t*)sessions.data(), sessions.size()});
}

void NetworkManagerImpl::sendRequestPlayerCounts(uint64_t session) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto reqr = msg.initRequestPlayerCounts();
        reqr.setLevels(kj::arrayPtr(&session, 1));
    });
}

void NetworkManagerImpl::sendCreateRoom(const std::string& name, uint32_t passcode, const RoomSettings& settings) {
    RoomManager::get().setAttemptedPasscode(passcode);

    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto createRoom = msg.initCreateRoom();
        createRoom.setName(name);
        createRoom.setPasscode(passcode);

        data::encode(settings, createRoom.initSettings());
    });
}

void NetworkManagerImpl::sendJoinRoom(uint32_t id, uint32_t passcode) {
    RoomManager::get().setAttemptedPasscode(passcode);

    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto joinRoom = msg.initJoinRoom();
        joinRoom.setRoomId(id);
        joinRoom.setPasscode(passcode);
    });
}

void NetworkManagerImpl::sendJoinRoomByToken(uint64_t token) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto joinRoom = msg.initJoinRoomByToken();
        joinRoom.setToken(token);
    });
}

void NetworkManagerImpl::sendLeaveRoom() {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        msg.setLeaveRoom();
    });
}

void NetworkManagerImpl::sendRequestRoomList(CStr nameFilter, uint32_t page) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto requestRoomList = msg.initRequestRoomList();
        requestRoomList.setNameFilter(nameFilter.get());
        requestRoomList.setPage(page);
    });
}

void NetworkManagerImpl::sendAssignTeam(int accountId, uint16_t teamId) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto assign = msg.initAssignTeam();
        assign.setAccountId(accountId);
        assign.setTeamId(teamId);
    });
}

void NetworkManagerImpl::sendCreateTeam(cocos2d::ccColor4B color) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto create = msg.initCreateTeam();
        create.setColor(data::encodeColor4(color));
    });
}

void NetworkManagerImpl::sendDeleteTeam(uint16_t teamId) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto del = msg.initDeleteTeam();
        del.setTeamId(teamId);
    });
}

void NetworkManagerImpl::sendUpdateTeam(uint16_t teamId, cocos2d::ccColor4B color) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto update = msg.initUpdateTeam();
        update.setTeamId(teamId);
        update.setColor(data::encodeColor4(color));
    });
}

void NetworkManagerImpl::sendGetTeamMembers() {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        msg.initGetTeamMembers();
    });
}

void NetworkManagerImpl::sendRoomOwnerAction(RoomOwnerActionType type, int target) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto roa = msg.initRoomOwnerAction();
        roa.setType(type);
        roa.setTarget(target);
    });
}

void NetworkManagerImpl::sendUpdateRoomSettings(const RoomSettings& settings) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto upd = msg.initUpdateRoomSettings();
        data::encode(settings, upd.initSettings());
    });
}

void NetworkManagerImpl::sendInvitePlayer(int32_t player) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto invp = msg.initInvitePlayer();
        invp.setPlayer(player);
    });
}

void NetworkManagerImpl::sendFetchCredits() {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        msg.initFetchCredits();
    });
}

void NetworkManagerImpl::sendGetDiscordLinkState() {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        msg.initGetDiscordLinkState();
    });
}

void NetworkManagerImpl::sendSetDiscordPairingState(bool state) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto m = msg.initSetDiscordPairingState();
        m.setState(state);
    });
}

void NetworkManagerImpl::sendDiscordLinkConfirm(int64_t id, bool confirm) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto m = msg.initDiscordLinkConfirm();
        m.setId(id);
        m.setAccept(confirm);
    });
}

void NetworkManagerImpl::sendGetFeaturedList(uint32_t page) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto m = msg.initGetFeaturedList();
        m.setPage(page);
    });
}

void NetworkManagerImpl::sendGetFeaturedLevel() {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        msg.setGetFeaturedLevel();
    });
}

void NetworkManagerImpl::sendSendFeaturedLevel(
    int32_t levelId,
    const std::string& levelName,
    int32_t authorId,
    const std::string& authorName,
    uint8_t rateTier,
    const std::string& note,
    bool queue
) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto m = msg.initSendFeaturedLevel();
        m.setLevelId(levelId);
        m.setLevelName(levelName);
        m.setAuthorId(authorId);
        m.setAuthorName(authorName);
        m.setRateTier(rateTier);
        m.setNote(note);
        m.setQueue(queue);
    });
}

void NetworkManagerImpl::sendNoticeReply(int32_t recipientId, const std::string& message) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto m = msg.initNoticeReply();
        m.setReceiverId(recipientId);
        m.setMessage(message);
    });
}

void NetworkManagerImpl::sendAdminNotice(const std::string& message, const std::string& user, int roomId, int levelId, bool canReply, bool showSender) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto adminNotice = msg.initAdminNotice();
        adminNotice.setMessage(message);
        adminNotice.setTargetUser(user);
        adminNotice.setRoomId(roomId);
        adminNotice.setLevelId(levelId);
        adminNotice.setCanReply(canReply);
        adminNotice.setShowSender(showSender);
    });
}

void NetworkManagerImpl::sendAdminNoticeEveryone(const std::string& message) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto adminNotice = msg.initAdminNoticeEveryone();
        adminNotice.setMessage(message);
    });
}

void NetworkManagerImpl::sendAdminLogin(const std::string& password) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto adminLogin = msg.initAdminLogin();
        adminLogin.setPassword(password);
    });
}

void NetworkManagerImpl::sendAdminKick(int32_t accountId, const std::string& message) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto adminLogin = msg.initAdminKick();
        adminLogin.setAccountId(accountId);
        adminLogin.setMessage(message);
    });
}

void NetworkManagerImpl::sendAdminFetchUser(const std::string& query) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto fetchUser = msg.initAdminFetchUser();
        fetchUser.setQuery(query);
    });
}

void NetworkManagerImpl::sendAdminFetchMods() {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto fetchUser = msg.initAdminFetchMods();
    });
}

void NetworkManagerImpl::sendAdminSetWhitelisted(int32_t accountId, bool whitelisted) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto m = msg.initAdminSetWhitelisted();
        m.setAccountId(accountId);
        m.setWhitelisted(whitelisted);
    });
}

void NetworkManagerImpl::sendAdminCloseRoom(uint32_t roomId) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto m = msg.initAdminCloseRoom();
        m.setRoomId(roomId);
    });
}

void NetworkManagerImpl::sendAdminFetchLogs(const FetchLogsFilters& filters) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto flogs = msg.initAdminFetchLogs();
        flogs.setIssuer(filters.issuer);
        flogs.setTarget(filters.target);
        flogs.setType({ filters.type.data(), filters.type.size() });
        flogs.setBefore(filters.before);
        flogs.setAfter(filters.after);
        flogs.setPage(filters.page);
    });
}

void NetworkManagerImpl::sendAdminBan(int32_t accountId, const std::string& reason, int64_t expiresAt) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto pun = msg.initAdminBan();
        pun.setAccountId(accountId);
        pun.setReason(reason);
        pun.setExpiresAt(expiresAt);
    });
}

void NetworkManagerImpl::sendAdminUnban(int32_t accountId) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto unp = msg.initAdminUnban();
        unp.setAccountId(accountId);
    });
}

void NetworkManagerImpl::sendAdminRoomBan(int32_t accountId, const std::string& reason, int64_t expiresAt) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto pun = msg.initAdminRoomBan();
        pun.setAccountId(accountId);
        pun.setReason(reason);
        pun.setExpiresAt(expiresAt);
    });
}

void NetworkManagerImpl::sendAdminRoomUnban(int32_t accountId) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto unp = msg.initAdminRoomUnban();
        unp.setAccountId(accountId);
    });
}

void NetworkManagerImpl::sendAdminMute(int32_t accountId, const std::string& reason, int64_t expiresAt) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto pun = msg.initAdminMute();
        pun.setAccountId(accountId);
        pun.setReason(reason);
        pun.setExpiresAt(expiresAt);
    });
}

void NetworkManagerImpl::sendAdminUnmute(int32_t accountId) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto unp = msg.initAdminUnmute();
        unp.setAccountId(accountId);
    });
}

void NetworkManagerImpl::sendAdminEditRoles(int32_t accountId, const std::vector<uint8_t>& roles) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto m = msg.initAdminEditRoles();
        m.setAccountId(accountId);
        m.setRoles({roles.data(), roles.size()});
    });
}

void NetworkManagerImpl::sendAdminSetPassword(int32_t accountId, const std::string& password) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto m = msg.initAdminSetPassword();
        m.setAccountId(accountId);
        m.setNewPassword(password);
    });
}

void NetworkManagerImpl::sendAdminUpdateUser(int32_t accountId, const std::string& username, int16_t cube, uint16_t color1, uint16_t color2, uint16_t glowColor) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto m = msg.initAdminUpdateUser();
        m.setAccountId(accountId);
        m.setUsername(username);
        m.setCube(cube);
        m.setColor1(color1);
        m.setColor2(color2);
        m.setGlowColor(glowColor);
    });
}

void NetworkManagerImpl::addListener(const std::type_info& ty, void* listener, void* dtor) {
    std::type_index index{ty};
    auto listeners = m_listeners.lock();
    auto& ls = (*listeners)[index];

    ls.push_back({listener, dtor});

    log::debug(
        "Added listener {} for message type '{}', priority: {}",
        listener,
        ty.name(),
        static_cast<MessageListenerImplBase*>(listener)->m_priority
    );

    // sort them by priority
    std::sort(ls.begin(), ls.end(), [](const auto& a, const auto& b) {
        auto* implA = static_cast<MessageListenerImplBase*>(a.first);
        auto* implB = static_cast<MessageListenerImplBase*>(b.first);
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
        for (auto pos = vec.begin(); pos != vec.end(); ++pos) {
            if (pos->first != listener) continue;
            auto dtor = reinterpret_cast<void(*)(void*)>(pos->second);

            vec.erase(pos);
            if (vec.empty()) {
                listeners->erase(it);
            }

            dtor(listener);
            break;
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

            auto res = data::decodeOpt<msg::CentralLoginOkMessage>(loginOk);
            if (!res) {
                return Err("invalid CentralLoginOkMessage");
            }

            auto msg = std::move(res).value();
            connInfo.m_allRoles = msg.allRoles;
            connInfo.m_userRoles = msg.userRoles;
            connInfo.m_established = true;
            connInfo.m_perms = msg.perms;
            connInfo.m_nameColor = msg.nameColor;
            connInfo.m_featuredLevel = msg.featuredLevel;

            for (auto& role : connInfo.m_userRoles) {
                connInfo.m_userRoleIds.push_back(role.id);
            }

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
            auto m = data::decodeUnchecked<msg::BannedMessage>(msg.getBanned());
            this->invokeListeners(m);
            this->abortConnection("User is banned from the server", true);
        } break;

        case CentralMessage::MUTED: {
            this->invokeListeners(data::decodeUnchecked<msg::MutedMessage>(msg.getMuted()));
        } break;

        case CentralMessage::SERVERS_CHANGED: {
            auto lock = m_connInfo.lock();
            auto& connInfo = **lock;

            auto serversChanged = msg.getServersChanged();
            auto servers = serversChanged.getServers();

            updateServers(connInfo.m_gameServers, servers);
        } break;

        case CentralMessage::USER_DATA_CHANGED: {
            auto outp = data::decodeUnchecked<msg::UserDataChangedMessage>(msg.getUserDataChanged());
            auto lock = m_connInfo.lock();
            auto& connInfo = **lock;

            connInfo.m_perms = outp.perms;
            connInfo.m_nameColor = outp.nameColor;
            connInfo.m_userRoleIds = outp.roles;

            connInfo.m_userRoles.clear();
            for (auto id : outp.roles) {
                if (id < connInfo.m_allRoles.size()) {
                    connInfo.m_userRoles.push_back(connInfo.m_allRoles[id]);
                } else {
                    log::warn("Unknown role ID: {}", id);
                }
            }

            this->invokeListeners(std::move(outp));
        } break;

        case CentralMessage::PLAYER_COUNTS: {
            this->invokeListeners(data::decodeUnchecked<msg::PlayerCountsMessage>(msg.getPlayerCounts()));
        } break;

        case CentralMessage::GLOBAL_PLAYERS: {
            this->invokeListeners(data::decodeUnchecked<msg::GlobalPlayersMessage>(msg.getGlobalPlayers()));
        } break;

        case CentralMessage::ROOM_PLAYERS: {
            this->invokeListeners(data::decodeUnchecked<msg::RoomPlayersMessage>(msg.getRoomPlayers()));
        } break;

        case CentralMessage::LEVEL_LIST: {
            this->invokeListeners(data::decodeUnchecked<msg::LevelListMessage>(msg.getLevelList()));
        } break;

        case CentralMessage::ROOM_STATE: {
            this->invokeListeners(data::decodeUnchecked<msg::RoomStateMessage>(msg.getRoomState()));
        } break;

        case CentralMessage::ROOM_JOIN_FAILED: {
            auto roomJoinFailed = msg.getRoomJoinFailed();

            if (auto msg = data::decodeOpt<msg::RoomJoinFailedMessage>(roomJoinFailed)) {
                this->invokeListeners(*msg);
            } else {
                log::warn("Received invalid room join failed message");
            }
        } break;

        case CentralMessage::ROOM_CREATE_FAILED: {
            auto roomCreateFailed = msg.getRoomCreateFailed();

            if (auto msg = data::decodeOpt<msg::RoomCreateFailedMessage>(roomCreateFailed)) {
                this->invokeListeners(*msg);
            } else {
                log::warn("Received invalid room create failed message");
            }
        } break;

        case CentralMessage::ROOM_BANNED: {
            this->invokeListeners(data::decodeUnchecked<msg::RoomBannedMessage>(msg.getRoomBanned()));
        } break;

        case CentralMessage::ROOM_LIST: {
            auto roomList = msg.getRoomList();

            if (auto msg = data::decodeOpt<msg::RoomListMessage>(roomList)) {
                this->invokeListeners(*msg);
            } else {
                log::warn("Received invalid room list message");
            }
        } break;

        case CentralMessage::TEAM_CREATION_RESULT: {
            this->invokeListeners(data::decodeUnchecked<msg::TeamCreationResultMessage>(msg.getTeamCreationResult()));
        } break;

        case CentralMessage::TEAM_CHANGED: {
            this->invokeListeners(data::decodeUnchecked<msg::TeamChangedMessage>(msg.getTeamChanged()));
        } break;

        case CentralMessage::TEAM_MEMBERS: {
            this->invokeListeners(data::decodeUnchecked<msg::TeamMembersMessage>(msg.getTeamMembers()));
        } break;

        case CentralMessage::TEAMS_UPDATED: {
            this->invokeListeners(data::decodeUnchecked<msg::TeamsUpdatedMessage>(msg.getTeamsUpdated()));
        } break;

        case CentralMessage::JOIN_FAILED: {
            log::warn("Received join failed message: {}", (int) msg.getJoinFailed().getReason());
        } break;

        case CentralMessage::ROOM_SETTINGS_UPDATED: {
            auto settings = msg.getRoomSettingsUpdated();
            auto rs = data::decodeUnchecked<RoomSettings>(settings.getSettings());

            this->invokeListeners(msg::RoomSettingsUpdatedMessage { rs });
        } break;

        case CentralMessage::INVITED: {
            this->invokeListeners(data::decodeUnchecked<msg::InvitedMessage>(msg.getInvited()));
        } break;

        case CentralMessage::INVITE_TOKEN_CREATED: {
            this->invokeListeners(data::decodeUnchecked<msg::InviteTokenCreatedMessage>(msg.getInviteTokenCreated()));
        } break;

        //

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
                .isReply = notice.getIsReply(),
            });
        } break;

        case CentralMessage::WARN: {
            // TODO
        } break;

        case CentralMessage::CREDITS: {
            this->invokeListeners(data::decodeUnchecked<msg::CreditsMessage>(msg.getCredits()));
        } break;

        case CentralMessage::DISCORD_LINK_STATE: {
            this->invokeListeners(data::decodeUnchecked<msg::DiscordLinkStateMessage>(msg.getDiscordLinkState()));
        } break;

        case CentralMessage::DISCORD_LINK_ATTEMPT: {
            this->invokeListeners(data::decodeUnchecked<msg::DiscordLinkAttemptMessage>(msg.getDiscordLinkAttempt()));
        } break;

        case CentralMessage::FEATURED_LEVEL: {
            auto out = data::decodeUnchecked<msg::FeaturedLevelMessage>(msg.getFeaturedLevel());
            (**m_connInfo.lock()).m_featuredLevel = out.meta;

            this->invokeListeners(std::move(out));
        } break;

        case CentralMessage::FEATURED_LIST: {
            this->invokeListeners(data::decodeUnchecked<msg::FeaturedListMessage>(msg.getFeaturedList()));
        } break;

        //

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
            this->invokeListeners(data::decodeUnchecked<msg::AdminFetchResponseMessage>(msg.getAdminFetchResponse()));
        } break;

        case CentralMessage::ADMIN_FETCH_MODS_RESPONSE: {
            this->invokeListeners(data::decodeUnchecked<msg::AdminFetchModsResponseMessage>(msg.getAdminFetchModsResponse()));
        } break;

        case CentralMessage::ADMIN_LOGS_RESPONSE: {
            this->invokeListeners(data::decodeUnchecked<msg::AdminLogsResponseMessage>(msg.getAdminLogsResponse()));
        } break;

        case CentralMessage::ADMIN_PUNISHMENT_REASONS: {
            auto m = data::decodeUnchecked<msg::AdminPunishmentReasonsMessage>(msg.getAdminPunishmentReasons());
            (**m_connInfo.lock()).m_punishReasons = m.reasons;
            this->invokeListeners(std::move(m));
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
            this->invokeListeners(data::decodeUnchecked<msg::LevelDataMessage>(msg.getLevelData()));
        } break;

        case GameMessage::SCRIPT_LOGS: {
            this->invokeListeners(data::decodeUnchecked<msg::ScriptLogsMessage>(msg.getScriptLogs()));
        } break;

        case GameMessage::VOICE_BROADCAST: {
            this->invokeListeners(data::decodeUnchecked<msg::VoiceBroadcastMessage>(msg.getVoiceBroadcast()));
        } break;

        case GameMessage::QUICK_CHAT_BROADCAST: {
            this->invokeListeners(data::decodeUnchecked<msg::QuickChatBroadcastMessage>(msg.getQuickChatBroadcast()));
        } break;

        case GameMessage::CHAT_NOT_PERMITTED: {
            this->invokeListeners(msg::ChatNotPermittedMessage{});
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
            this->abortConnection("Internal server error (auth system is failing), please contact the developer!");
        } break;

        case INTERNAL_DB_ERROR: {
            log::warn("Login failed: internal database error");
            this->abortConnection("Internal server error (database error), please contact the developer!");
        } break;

        case INVALID_ACCOUNT_DATA: {
            log::warn("Login failed: invalid account data");
            this->abortConnection("Internal server error (invalid account data), please contact the developer!");
        } break;

        case NOT_WHITELISTED: {
            log::warn("Login failed: user is not whitelisted");
            this->abortConnection("You are not whitelisted on this server!");
        } break;

        default: {
            log::warn("Login failed: unknown reason {}", static_cast<int>(reason));
            this->abortConnection(fmt::format("Login failed due to unknown server error: {}", static_cast<int>(reason)));
        } break;
    }
}

void NetworkManagerImpl::disconnectInner() {
    m_disconnectRequested.store(true);
    m_disconnectNotify.notifyOne();
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

int32_t NetworkManagerImpl::getLastFeaturedLevelId() {
    return globed::value<int32_t>(fmt::format("core.last-featured-id.{}", this->getCentralIdent())).value_or(0);
}

void NetworkManagerImpl::setLastFeaturedLevelId(int32_t id) {
    ValueManager::get().set(fmt::format("core.last-featured-id.{}", this->getCentralIdent()), id);
}

std::vector<uint8_t> NetworkManagerImpl::computeUident(int accountId) {
    static asp::Mutex<std::pair<int, std::vector<uint8_t>>> mtx;

    auto lock = mtx.lock();
    if (lock->first == accountId) {
        return lock->second;
    }

    uint8_t buf[512];
    size_t outLen = bb_work(accountId, (char*)buf, 512);

    if (outLen == 0) {
        log::error("Failed to compute uident!");
        return {};
    }

    lock->first = accountId;
    lock->second = std::vector<uint8_t>{buf, buf + outLen};

    return lock->second;
}

}
