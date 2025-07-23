#include "NetworkManagerImpl.hpp"
#include <globed/core/ValueManager.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/core/net/NetworkManager.hpp>
#include <argon/argon.hpp>
#include <core/CoreImpl.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>

#include "data/helpers.hpp"

using namespace geode::prelude;

static qn::ConnectionDebugOptions getConnOpts() {
    qn::ConnectionDebugOptions opts{};
    opts.packetLossSimulation = globed::setting<float>("core.dev.packet-loss-sim");

    if (opts.packetLossSimulation != 0.f) {
        log::warn("Packet loss simulation enabled: {}%", opts.packetLossSimulation * 100.f);
    }

    return opts;
}

namespace globed {

NetworkManagerImpl::NetworkManagerImpl() {
    m_centralConn.setConnectionStateCallback([this](qn::ConnectionState state) {
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
        if (state == qn::ConnectionState::Connected) {
            log::debug("connected to game server at {}", m_gameServerUrl);
            m_gameEstablished = true;

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
}

NetworkManagerImpl::~NetworkManagerImpl() {}

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

std::optional<uint8_t> NetworkManagerImpl::getPreferredServer() {
    if (!m_established) {
        return std::nullopt;
    }

    if (auto value = globed::value<std::string>("preferred-server")) {
        for (auto& srv : m_gameServers) {
            if (srv.second.stringId == *value) {
                return srv.second.id;
            }
        }
    }

    // fallback to the server with the lowest latency
    if (!m_gameServers.empty()) {
        auto it = std::min_element(m_gameServers.begin(), m_gameServers.end(),
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

void NetworkManagerImpl::onCentralConnected() {
    log::debug("connection to central server established, trying to log in");
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
    m_gameServers.clear();
    m_knownArgonUrl.clear();
}

void NetworkManagerImpl::joinSession(SessionId id) {
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
    for (auto& srv : m_gameServers) {
        if (srv.second.id == serverId) {
            this->joinSessionWith(srv.second.url, id);
            return;
        }
    }

    // not found!
    log::error("Tried joining an invalid session, game server with ID {} does not exist", static_cast<int>(serverId));
}

void NetworkManagerImpl::leaveSession() {
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
    });
}

void NetworkManagerImpl::sendGameLoginRequest() {
    (void) this->sendToGame([&](GameMessage::Builder& msg) {
        auto login = msg.initLoginUToken();
        login.setAccountId(GJAccountManager::get()->m_accountID);
        login.setToken(this->getUToken().value_or(""));
    });
}

void NetworkManagerImpl::sendGameJoinRequest(SessionId id) {
    (void) this->sendToGame([&](GameMessage::Builder& msg) {
        auto join = msg.initJoinSession();
        join.setSessionId(id);
        join.setPasscode(0); // TODO
    });
}

void NetworkManagerImpl::sendPlayerState(const PlayerState& state) {
    (void) this->sendToGame([&](GameMessage::Builder& msg) {
        auto playerData = msg.initPlayerData();
        auto data = playerData.initData();
        data::encodePlayerState(state, data);
    }, false);
}

static void updateServers(std::unordered_map<std::string, GameServer>& servers, auto& newServers) {
    servers.clear();

    for (auto srv : newServers) {
        GameServer gameServer;
        gameServer.id = srv.getId();
        gameServer.stringId = srv.getStringId();
        gameServer.url = srv.getAddress();
        gameServer.name = srv.getName();
        gameServer.region = srv.getRegion();

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
                updateServers(m_gameServers, servers);
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

        case GameMessage::JOIN_SESSION_OK: {} break;

        case GameMessage::JOIN_SESSION_FAILED: {
            using enum schema::game::JoinSessionFailedReason;

            auto reason = msg.getJoinSessionFailed().getReason();

            switch (reason) {
                case INVALID_PASSCODE: {
                    log::warn("Join session failed: invalid passcode");
                } break;
            }
        } break;

        case GameMessage::LEVEL_DATA: {
            auto data = msg.getLevelData();
            auto players = data.getPlayers();
            auto culled = data.getCulled();

            std::vector<PlayerState> states;
            std::vector<int> culledIds;

            states.reserve(players.size());
            culledIds.reserve(culled.size());

            for (auto player : players) {
                if (auto s = data::decodePlayerState(player)) {
                    states.emplace_back(*s);
                } else {
                    log::warn("Server sent invalid player state data for {}, skipping", player.getAccountId());
                }
            }

            for (auto id : culled) {
                culledIds.push_back(id);
            }

            // TODO (high): better way to dispatch packets
            Loader::get()->queueInMainThread([states = std::move(states), culledIds = std::move(culledIds)] mutable {
                if (auto gjbgl = GlobedGJBGL::get()) {
                    gjbgl->onLevelDataReceived(states, culledIds);
                }
            });
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
