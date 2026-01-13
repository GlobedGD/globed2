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
#include <globed/util/scary.hpp>
#include <core/CoreImpl.hpp>
#include "data/helpers.hpp"
#include <bb_public.hpp>

#include <kj/exception.h>
#include <arc/future/Select.hpp>
#include <arc/time/Sleep.hpp>
#include <argon/argon.hpp>
#include <qunet/Pinger.hpp>
#include <qunet/dns/Resolver.hpp>
#include <qunet/util/algo.hpp>
#include <qunet/util/hash.hpp>
#include <asp/time/Duration.hpp>
#include <asp/iter.hpp>

using namespace geode::prelude;
using enum std::memory_order;
using namespace asp::time;
using namespace arc;

static qn::ConnectionDebugOptions getConnOpts() {
    qn::ConnectionDebugOptions opts{};
    opts.packetLossSimulation = globed::setting<float>("core.dev.packet-loss-sim");

    if (opts.packetLossSimulation != 0.f) {
        log::warn("Packet loss simulation enabled: {}%", opts.packetLossSimulation * 100.f);
    }

    opts.recordStats = globed::setting<bool>("core.dev.net-stat-dump");

    return opts;
}

static void gatherUserSettings(auto&& out) {
    out.setHideInLevel(globed::setting<bool>("core.user.hide-in-levels"));
    out.setHideInMenus(globed::setting<bool>("core.user.hide-in-menus"));
    out.setHideRoles(globed::setting<bool>("core.user.hide-roles"));
    out.setDisableNotices(globed::setting<bool>("core.ui.disable-notices"));
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

static std::string_view connectionStateToStr(qn::ConnectionState state) {
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

static std::string_view connTypeToString(qn::ConnectionType type) {
    using enum qn::ConnectionType;

    switch (type) {
        case Udp: return "udp";
        case Tcp: return "tcp";
        case Quic: return "quic";
        default: return "unknown";
    }
}

// addr/type/error priority is as follows:
// * TransportError -> handshake failure / defragmentation / unreliable / invalid qdb
// * IPv4 over IPv6
// * prefer tcp over udp over quic
// * non socket error
// * socket error
static int rankError(const qsox::SocketAddress& addr, qn::ConnectionType type, const qn::ConnectionError& err) {
    int score = 0;

    if (err.isTransportError()) {
        using Code = qn::TransportError::CustomCode;
        auto& te = err.asTransportError();

        if (std::holds_alternative<qn::TransportError::HandshakeFailure>(te.m_kind)) {
            score += 100'000'000;
        } else if (std::holds_alternative<qn::TransportError::CustomKind>(te.m_kind)) {
            auto code = std::get<qn::TransportError::CustomKind>(te.m_kind).code;
            if (code == Code::DefragmentationError || code == Code::TooUnreliable || code == Code::InvalidQunetDatabase) {
                score += 100'000'000;
            }
        }
    }

    if (addr.isV4()) {
        score += 50'000'000;
    }

    if (type == qn::ConnectionType::Tcp) {
        score += 25'000'000;
    } else if (type == qn::ConnectionType::Udp) {
        score += 24'000'000;
    } else if (type == qn::ConnectionType::Quic) {
        score += 23'000'000;
    }

    if (err.isTransportError()) {
        auto& te = err.asTransportError();

        if (!std::holds_alternative<qsox::Error>(te.m_kind)) {
            score += 10'000'000;
        } else {
            score += 1'000'000;
        }
    }

    return score;
};

struct CapnpExceptionHandler : public kj::ExceptionCallback {
    bool errored = false;

    void onRecoverableException(kj::Exception&& exception) override {
        log::warn("Capnp exception: {}", exception.getDescription().cStr());
        errored = true;
    }

    void onFatalException(kj::Exception&& exception) override {
        utils::terminate(fmt::format("Fatal capnp exception: {}", exception.getDescription().cStr()));
    }

    void logMessage(kj::LogSeverity severity, const char* file, int line, int contextDepth, kj::String&& text) override {
        log::debug("(capnp) {}", text.cStr());
    }
};

// Custom awaiter to bridge together geode tasks and arc futures
template <typename T, typename P>
struct GeodeTaskAwaiter : public arc::PollableBase<GeodeTaskAwaiter<T, P>, T> {
    using GeodeTask = geode::Task<T, P>;

    GeodeTaskAwaiter(GeodeTask task) : m_state(State::Init), m_task(std::move(task)) {}
    GeodeTaskAwaiter(GeodeTaskAwaiter&&) = delete;
    GeodeTaskAwaiter& operator=(GeodeTaskAwaiter&&) = delete;
    GeodeTaskAwaiter(const GeodeTaskAwaiter&) = delete;
    GeodeTaskAwaiter& operator=(const GeodeTaskAwaiter&) = delete;

    std::optional<T> poll() {
        switch (m_state) {
            case State::Init: {
                m_state = State::Waiting;
                m_waker = ctx().cloneWaker();
                m_listener.bind([this](GeodeTask::Event* event) mutable {
                    if (T* result = event->getValue()) {
                        // task finished
                        m_output = std::move(*result);
                        m_state = State::Done;
                        m_waker->wake();
                    } else if (event->isCancelled()) {
                        // task was cancelled
                        m_state = State::Done;
                        m_waker->wake();
                    }
                });
                m_listener.setFilter(m_task);
            } break;

            case State::Waiting: {
                return std::nullopt;
            } break;

            case State::Done: {
                auto out = std::move(m_output);
                m_output.reset();
                return out;
            } break;
        }

        return std::nullopt;
    }

private:
    enum class State {
        Init,
        Waiting,
        Done,
    } m_state;
    GeodeTask m_task;
    EventListener<GeodeTask> m_listener;
    std::optional<T> m_output;
    std::optional<Waker> m_waker;
};

static argon::AccountData g_argonData{};

namespace globed {

static Future<Result<std::string>> startArgonAuth() {
    auto task = argon::startAuthWithAccount(g_argonData);
    GeodeTaskAwaiter awaiter{task};
    co_return co_await awaiter;
}

static void updateServers(ConnectionInfo& info, auto& newServers) {
    info.m_gameServers.clear();

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

        info.m_gameServers[gameServer.stringId] = std::move(gameServer);
    }

    info.m_gameServersUpdated = true;
}

bool GameServer::updateLatency(uint32_t latency) {
    if (avgLatency == (uint32_t)-1) {
        avgLatency = latency;
    } else {
        avgLatency = qn::exponentialMovingAverage(avgLatency, latency, 0.4);
    }

    lastLatency = latency;

    // consider unstable if the jitter is significant enough
    return std::abs((int32_t)lastLatency - (int32_t)avgLatency) > (int32_t)(avgLatency * 0.5f);
}

WorkerState createWorkerState() {
    auto [tx, rx] = arc::mpsc::channel<std::pair<std::string, qn::PingResult>>(32);
    return WorkerState{std::move(tx), std::move(rx)};
}

NetworkManagerImpl::NetworkManagerImpl() : m_runtime(Runtime::create(2)), m_workerState(createWorkerState()) {
    m_hasSecure = bb_init();

    m_runtime->setTerminateHandler([](const std::exception& e) {
        utils::terminate(fmt::format(
            "arc runtime terminated due to unhandled exception: {}",
            e.what()
        ));
    });

    m_runtime->spawn([](auto* self) -> arc::Future<> {
        co_await self->asyncInit();
    }(this));
}

NetworkManagerImpl::~NetworkManagerImpl() {
    // m_centralConn->destroy();
    // m_gameConn->destroy();
    m_destructing = true;
}

void NetworkManagerImpl::shutdown() {
    log::debug("shutting down networking...");

    // set up a notify so we know when both connections are fully disconnected
    arc::Notify notify;
    m_centralConn->setConnectionStateCallback([&](qn::ConnectionState state) {
        notify.notifyOne();
    });
    m_gameConn->setConnectionStateCallback([&](qn::ConnectionState state) {
        notify.notifyOne();
    });

    m_centralConn->disconnect();
    m_gameConn->disconnect();

    // allow some short cleanup
    (void) m_runtime->blockOn(
        arc::timeout(
            Duration::fromMillis(100),
            [&](this auto self) -> arc::Future<> {
                while (true) {
                    auto cs = m_centralConn->state();
                    auto gs = m_gameConn->state();

                    if (cs == gs && cs == qn::ConnectionState::Disconnected) {
                        break;
                    }

                    co_await notify.notified();
                }
            }()
        )
    );

    m_centralConn->destroy();
    m_gameConn->destroy();
    m_centralConn.reset();
    m_gameConn.reset();

    m_centralLogger.reset();
    m_gameLogger.reset();
    m_gameServerJoinTx.reset();

    m_runtime.reset();

    log::debug("network cleanup finished!");
}

NetworkManagerImpl& NetworkManagerImpl::get() {
    return *NetworkManager::get().m_impl;
}

Future<> NetworkManagerImpl::asyncInit() {
    // this does not have to be async but it also must not be in the constructor
    if (globed::value<bool>("net.dont-override-dns").value_or(false)) {
        log::info("Not overriding DNS servers");
    } else {
        qn::Resolver::get().setCustomDnsServers(
            qsox::Ipv4Address{1, 1, 1, 1},
            qsox::Ipv4Address{8, 8, 8, 8}
        );
    }

    auto [gstx, gsrx] = arc::mpsc::channel<GameServerJoinRequest>(4);
    m_gameServerJoinTx = std::move(gstx);
    m_gameWorkerState.joinRx = std::move(gsrx);

    m_centralConn = co_await qn::Connection::create();
    m_gameConn = co_await qn::Connection::create();

    // TODO (low): measure how much of an impact those have on bandwidth
    m_centralConn->setActiveKeepaliveInterval(Duration::fromSecs(45));
    m_gameConn->setActiveKeepaliveInterval(Duration::fromSecs(10));

    auto qdbPath = Mod::get()->getSaveDir() / "qdb-cache";
    m_centralConn->setQdbFolder(qdbPath);
    m_gameConn->setQdbFolder(qdbPath);

    m_centralConn->setConnectionStateCallback([this](qn::ConnectionState state) {
        this->onCentralStateChanged(state);
    });

    m_centralConn->setDataCallback([this](std::vector<uint8_t> bytes) {
        if (m_centralLogger) {
            m_centralLogger->sendPacketLog(bytes, false);
        }

        qn::ByteReader breader{bytes};
        size_t unpackedSize = breader.readVarUint().unwrapOr(-1);

        CapnpExceptionHandler errHandler;
        kj::ArrayInputStream ais{{bytes.data() + breader.position(), bytes.size() - breader.position()}};
        capnp::PackedMessageReader reader{ais};
        CentralMessage::Reader msg = reader.getRoot<CentralMessage>();

        if (errHandler.errored) {
            log::error("capnp error while reading central message, dropping");
            return;
        }

        if (auto err = this->onCentralDataReceived(msg).err()) {
            log::error("failed to process message from central server: {}", *err);
        }
    });

    m_centralConn->setStateResetCallback([this] {
        log::debug("State reset callback invoked, assuming stateless reconnect happened");
        auto info = m_connInfo.lock();
        info->reset();
    });

    m_gameConn->setConnectionStateCallback([this](qn::ConnectionState state) {
        this->onGameStateChanged(state);
    });

    m_gameConn->setDataCallback([this](std::vector<uint8_t> bytes) {
        if (m_gameLogger) {
            m_gameLogger->sendPacketLog(bytes, false);
        }

        qn::ByteReader breader{bytes};
        size_t unpackedSize = breader.readVarUint().unwrapOr(-1);

        CapnpExceptionHandler errHandler;
        kj::ArrayInputStream ais{{bytes.data() + breader.position(), bytes.size() - breader.position()}};
        capnp::PackedMessageReader reader{ais};
        GameMessage::Reader msg = reader.getRoot<GameMessage>();

        if (errHandler.errored) {
            log::error("capnp error while reading game message, dropping");
            return;
        }

        if (auto err = this->onGameDataReceived(msg).err()) {
            log::error("failed to process message from game server: {}", err);
        }
    });

    // Spawn the main worker tasks
    arc::spawn([](auto* self) -> arc::Future<> {
        while (true) {
            co_await self->threadWorkerLoop();
        }
    }(this));

    arc::spawn([](auto* self) -> arc::Future<> {
        while (true) {
            co_await self->threadGameWorkerLoop();
        }
    }(this));
}

Future<> NetworkManagerImpl::threadWorkerLoop() {
    using enum qn::ConnectionState;

    // might happen when exiting, just wait and return (to not busy loop)
    if (!m_centralConn) {
        co_await arc::sleep(Duration::fromMillis(100));
        co_return;
    }

    // Worker task has the following duties:
    // - Watch the connection state
    // - When connected, periodically ping game servers and decide to resend own data
    auto connState = m_centralConn->state();
    auto prevState = m_workerState.prevCentralState;
    m_workerState.prevCentralState = connState;

    // reset connection info on disconnect, and create it when needed
    if (connState == Disconnected) {
        m_connInfo.lock()->reset();
    } else if (connState == Connected && connState != prevState) {
        auto info = m_connInfo.lock();
        if (!*info) info->emplace();
    }

    switch (connState) {
        case Connected: {
            // if we weren't connected before, do some initial setup
            if (prevState != qn::ConnectionState::Connected) {
                m_workerState.nextGSPing = Instant::now();
                m_workerState.pingResultRx.drain();
                globed::setValue<bool>("core.was-connected", true);

                co_await this->threadSetupLogger(true);
            }

            {
                auto info = this->connInfo();

                // if we aren't authenticated yet, try to do auth
                if (!info->m_established && !info->m_authenticating) {
                    info.unlock();
                    co_await this->threadTryAuth();
                    co_return;
                }

                // check if icons or friend list need to be resent
                this->threadMaybeResendOwnData(info);

                // reset the timer to ping game servers if the server list was updated
                if (info->m_gameServersUpdated) {
                    log::debug("Pinging servers again soon");
                    info->m_gameServersUpdated = false;
                    m_workerState.nextGSPing = Instant::now();
                }
            }

            // wait for someone to notify us or timeout for one of the tasks
            co_await arc::select(
                arc::selectee(m_workerNotify.notified()),

                arc::selectee(
                    m_workerState.pingResultRx.recv(),
                    [&](auto result) -> arc::Future<> {
                        // this should never fail
                        auto [srvkey, res] = std::move(result).unwrap();

                        // update server latency
                        auto info = this->connInfo();
                        if (!info) co_return;

                        auto& servers = info->m_gameServers;
                        auto it = servers.find(srvkey);
                        if (it != servers.end()) {
                            auto lat = (uint32_t)res.responseTime.millis();
                            bool unstable = it->second.updateLatency(lat);
                            log::debug(
                                "Ping to server {} arrived, latency: {}ms avg, {}ms last, stable: {}",
                                srvkey,
                                it->second.avgLatency,
                                it->second.lastLatency,
                                unstable ? "no" : "yes"
                            );

                            // if at least one server is unstable, ping all servers again soon
                            if (unstable) {
                                log::debug("Scheduling a ping soon due to unstable results!");
                                m_workerState.nextGSPing = std::min(
                                    Instant::now() + Duration::fromSecs(5),
                                    m_workerState.nextGSPing
                                );
                            }
                        }
                    }
                ),

                arc::selectee(
                    arc::sleepUntil(m_workerState.nextGSPing),
                    [&] {
                        auto info = this->connInfo();
                        this->threadPingGameServers(info);
                    }
                )
            );
        } break;

        default: {
            // show a disconnect popup when reconnecting or disconnected
            bool showDisconnect = false;
            if ((connState == Reconnecting || connState == Disconnected) && prevState != connState) {
                showDisconnect = true;
            }

            // wasConnected is true if we were connected before losing connection
            bool wasConnected = prevState == Connected || (prevState == Reconnecting && connState == Disconnected);

            if (wasConnected) {
                this->threadFlushLogger(true);
            }

            if (showDisconnect) {
                this->showDisconnectCause(connState == Reconnecting, wasConnected);
                co_return;
            }

            // do nothing, wait for something to happen
            co_await m_workerNotify.notified();
        } break;
    }

    co_return;
}

Future<> NetworkManagerImpl::threadGameWorkerLoop() {
    using enum qn::ConnectionState;

    // might happen when exiting, just wait and return (to not busy loop)
    if (!m_gameConn) {
        co_await arc::sleep(Duration::fromMillis(100));
        co_return;
    }

    auto connState = m_gameConn->state();
    auto prevState = m_gameWorkerState.prevGameState;
    m_gameWorkerState.prevGameState = connState;

    // do some logger things if connected / disconnected
    if (connState == Connected && connState != prevState) {
        co_await this->threadSetupLogger(false);
    } else if (connState == Disconnected && connState != prevState) {
        this->threadFlushLogger(false);
    }

    auto& cur = m_gameWorkerState.currentReq;
    if (cur) {
        bool sameUrl, gameEstablished;
        {
            auto info = this->connInfo();
            if (!info) {
                cur.reset();
                co_return;
            }

            sameUrl = info->m_gameServerUrl == cur->url;
            gameEstablished = info->m_gameEstablished;
        }

        if (connState == Connected) {
            if (!sameUrl) {
                // connected to a different server, disconnect and then connect to the needed one
                m_gameConn->disconnect();
            } else if (gameEstablished) {
                // connected to the same server as requested, simply send a join request
                this->sendGameJoinRequest(cur->id, cur->platformer, cur->editorCollab);
                cur.reset();
            } else {
                // connected but not yet logged in, send a login request
                this->sendGameLoginRequest(cur->id, cur->platformer, cur->editorCollab);
                cur.reset();
            }
        } else if (connState != Disconnected) {
            // connecting or closing, wait
        } else if (!cur->triedConnecting) {
            // disconnected, connect to the requested server
            m_gameConn->setDebugOptions(getConnOpts());
            auto res = m_gameConn->connect(cur->url);
            auto info = this->connInfo();

            if (!res) {
                log::error("Failed to connect to game server {}: {}", cur->url, res.unwrapErr().message());
                info->m_gameServerUrl.clear();
                cur.reset();
            } else {
                info->m_gameServerUrl = cur->url;
            }
        } else {
            // already tried connecting to this server and failed, so give it up
            this->connInfo()->m_gameServerUrl.clear();
            cur.reset();
        }
    }

    co_await arc::select(
        arc::selectee(m_gameWorkerNotify.notified()),

        // only poll for new join requests if there is no current request
        arc::selectee(m_gameWorkerState.joinRx->recv(), [&](auto result) {
            if (!result) return;
            auto req = std::move(result).unwrap();
            m_gameWorkerState.currentReq = std::move(req);
        }, !m_gameWorkerState.currentReq)
    );
}

void NetworkManagerImpl::showDisconnectCause(bool reconnecting, bool wasConnected) {
    bool showPopup = !m_manualDisconnect.load(::acquire);

    std::string_view messageStart = wasConnected ? "Connection lost" : "Failed to connect to the server";
    std::string message = fmt::format("{}: <cy>client initiated disconnect</c>", messageStart);

    if (!showPopup) {
        // only save the was-connected value if the user manually disconnected
        if (!m_destructing) {
            globed::setValue<bool>("core.was-connected", false);
        }
    } else {
        auto abortCause = m_abortCause.lock();

        if (!abortCause->first.empty()) {
            message = fmt::format("Connection aborted: <cy>{}</c>", abortCause->first);
        } else {
            auto err = m_centralConn->lastError();
            if (err == qn::ConnectionError::Success) {
                message = "Connection failed due to unknown error";
            } else if (!err.isAllAddressesFailed()) {
                message = fmt::format("{}: <cy>{}</c>", messageStart, err.message());
            } else {
                // if this is the AllAddressesFailed error, try to get more info to show
                // sort all errors and pick the clearest one to show to the user
                auto addrs = err.asAllAddressesFailed().addresses;
                std::sort(addrs.begin(), addrs.end(), [&](auto& a, auto& b) {
                    auto aRank = rankError(std::get<0>(a), std::get<1>(a), std::get<2>(a));
                    auto bRank = rankError(std::get<0>(b), std::get<1>(b), std::get<2>(b));
                    return aRank < bRank;
                });

                auto& [addr, type, cause] = addrs.back();
                message = fmt::format(
                    "Error connecting to <cg>{} ({})</c>: <cy>{}</c>.",
                    addr.toString(),
                    connTypeToString(type),
                    cause.message()
                );
            }
        }

        // if this was a silent abort, don't show a popup
        if (abortCause->second) {
            showPopup = false;
        }
    }

    log::info("connection to central server lost: {}", message);

    FunctionQueue::get().queue([reconnecting, showPopup, message = std::move(message)] {
        CoreImpl::get().onServerDisconnected();

        if (showPopup) {
            if (reconnecting) {
                // if reconnecting, show a toast instead
                globed::toastError("Globed: connection lost, reconnecting..");
            } else {
                auto alert = PopupManager::get().alert(
                    "Globed Error",
                    message
                );
                alert.showQueue();
            }
        }
    });
}

void NetworkManagerImpl::threadPingGameServers(LockedConnInfo& info) {
    Instant now = Instant::now();
    Instant nextPing = Instant::farFuture();

    for (auto& [srvkey, server] : info->m_gameServers) {
        auto interval = getPingInterval(server.sentPings);
        auto elapsed = now.durationSince(server.lastPingTime);

        if (elapsed >= interval) {
            // ping the server
            server.sentPings++;
            server.lastPingTime = now;

            auto tx = m_workerState.pingResultTx;

            auto res = qn::Pinger::get().pingUrl(server.url, [tx, srvkey = srvkey](qn::PingResult res) mutable {
                if (res.timedOut) {
                    log::debug("Ping to server {} timed out", srvkey);
                    return;
                }

                (void) tx.trySend({srvkey, res});
            });

            if (!res) {
                log::warn("Failed to ping game server {} ({}, id {}): {}", server.url, server.name, (int)server.id, res.unwrapErr());
            }

            interval = getPingInterval(server.sentPings);
        }

        nextPing = std::min(nextPing, server.lastPingTime + interval);
    }

    log::debug(
        "Scheduling next game server ping in {:.3f}s",
        (nextPing.durationSince(now)).seconds<float>()
    );
    m_workerState.nextGSPing = nextPing;
}

void NetworkManagerImpl::threadMaybeResendOwnData(LockedConnInfo& info) {
    // don't send if we haven't authorized yet
    if (!info->m_established) {
        return;
    }

    // check if icons or friend list need to be resent
    auto& flm = FriendListManager::get();
    bool friendList = !info->m_sentFriendList && flm.isLoaded();
    bool icons = !info->m_sentIcons;

    if (!icons && !friendList) {
        return;
    }


    // send own data

    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto update = msg.initUpdateOwnData();

        if (icons) {
            data::encode(PlayerIconData::getOwn(), update.initIcons());
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

    info->m_sentIcons = true;
    info->m_sentFriendList = true;
}

Future<> NetworkManagerImpl::threadSetupLogger(bool central) {
    if (!globed::setting<bool>("core.dev.net-stat-dump")) {
        m_centralLogger.reset();
        m_gameLogger.reset();
        co_return;
    }

    auto& opt = central ? m_centralLogger : m_gameLogger;
    if (!opt) {
        opt.emplace();

        auto base = Mod::get()->getConfigDir() / (central ? "central-logs" : "game-logs");
        co_await opt->setup(std::move(base));
    }

    opt->reset();
}

void NetworkManagerImpl::threadFlushLogger(bool central) {
    auto& opt = central ? m_centralLogger : m_gameLogger;
    if (opt) {
        opt->reset();
    }
}

Future<> NetworkManagerImpl::threadTryAuth() {
    if (auto stoken = this->getUToken()) {
        this->connInfo()->startedAuth();
        this->sendCentralAuth(AuthKind::Utoken, *stoken);
        co_return;
    }

    // try argon / plain auth
    auto info = this->connInfo();

    if (!info->m_knownArgonUrl.empty()) {
        (void) argon::setServerUrl(info->m_knownArgonUrl);

        // wait to acquire an argon token
        info.unlock();
        log::debug("acquiring argon token with url {}", info->m_knownArgonUrl);
        auto res = co_await startArgonAuth();

        if (!res) {
            this->abortConnection(fmt::format("failed to complete Argon auth: {}", res.unwrapErr()));
            co_return;
        }

        info.relock();
        info->startedAuth();
        this->sendCentralAuth(AuthKind::Argon, *res);
    } else {
        info->startedAuth();
        this->sendCentralAuth(AuthKind::Plain);
    }
}

void NetworkManagerImpl::sendCentralAuth(AuthKind kind, const std::string& token) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        int accountId = g_argonData.accountId;
        int userId = g_argonData.userId;

        auto login = msg.initLogin();
        login.setAccountId(accountId);
        data::encode(PlayerIconData::getOwn(), login.initIcons());
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
                plain.setUsername(g_argonData.username);
            } break;
        }
    });
}

Result<> NetworkManagerImpl::sendMessageToConnection(
    qn::Connection& conn,
    std::optional<ConnectionLogger>& logger,
    capnp::MallocMessageBuilder& msg,
    bool reliable,
    bool uncompressed
) {
    if (!conn.connected()) {
        return Err("not connected");
    }

    size_t unpackedSize = capnp::computeSerializedSizeInWords(msg) * 8;
    qn::ArrayByteWriter<8> writer;
    writer.writeVarUint(unpackedSize).unwrap();
    auto unpSizeBuf = writer.written();

    kj::VectorOutputStream vos;
    vos.write(unpSizeBuf.data(), unpSizeBuf.size());
    capnp::writePackedMessage(vos, msg);

    auto data = std::vector<uint8_t>(vos.getArray().begin(), vos.getArray().end());
    if (logger) {
        logger->sendPacketLog(data, true);
    }

    if (!conn.sendData(std::move(data), reliable, uncompressed)) {
        return Err("failed to send data");
    }

    return Ok();
}

void NetworkManagerImpl::sendToCentral(std23::function_ref<void(CentralMessage::Builder&)>&& func) {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<CentralMessage>();
    func(root);

    auto res = sendMessageToConnection(*m_centralConn, m_centralLogger, msg, true, false);

    if (!res) {
        log::warn("Failed to send message to central server: {}", res.unwrapErr());
    }
}

void NetworkManagerImpl::sendToGame(std23::function_ref<void(GameMessage::Builder&)>&& func, bool reliable, bool uncompressed) {
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<GameMessage>();
    func(root);

    auto res = sendMessageToConnection(*m_gameConn, m_gameLogger, msg, reliable, uncompressed);

    if (!res) {
        log::warn("Failed to send message to game server: {}", res.unwrapErr());
    }
}

LockedConnInfo NetworkManagerImpl::connInfo() const {
    return LockedConnInfo{m_connInfo.lock()};
}

Result<> NetworkManagerImpl::connectCentral(std::string_view url) {
    if (!m_centralConn->disconnected()) {
        return Err("Already connected to central server");
    }

    FriendListManager::get().refresh();

    bool certVerification = globed::setting<bool>("core.dev.cert-verification");
    argon::setCertVerification(certVerification);
    m_centralConn->setTlsCertVerification(certVerification);
    m_gameConn->setTlsCertVerification(certVerification);

    g_argonData = argon::getGameAccountData();
    m_abortCause.lock()->first.clear();
    m_manualDisconnect.store(false, ::release);

    m_centralConn->setDebugOptions(getConnOpts());
    auto res = m_centralConn->connect(std::string(url));

    if (!res) {
        FunctionQueue::get().queue([url, err = std::move(res).unwrapErr()] mutable {
            globed::alertFormat(
                "Globed Error",
                "Failed to connect to central server at <cp>'{}'</c>: <cy>{}</c>",
                url, err.message()
            );
        });

        return Err("Connection to {} failed: {}", url, res.unwrapErr().message());
    }

    return Ok();
}

void NetworkManagerImpl::disconnectCentral() {
    m_manualDisconnect.store(true, ::release);
    m_centralConn->disconnect();
    m_gameConn->disconnect();
}

qn::ConnectionState NetworkManagerImpl::getConnState(bool game) {
    if (game) {
        return m_gameConn->state();
    } else {
        return m_centralConn->state();
    }
}

void NetworkManagerImpl::dumpNetworkStats() {
    if (!globed::setting<bool>("core.dev.net-stat-dump")) return;

    auto describe = [](const qn::StatWholeSnapshot& snap) {
        log::info("> Connection duration: {}", snap.period.toString());
        log::info("> Bytes sent: {}, received: {}", snap.bytesUp, snap.bytesDown);
        log::info("> Messages sent: {}, received: {}", snap.messagesUp, snap.messagesDown);
        log::info("> Packets sent: {}, received: {}", snap.packetsUp, snap.packetsDown);
        log::info("> Compressed messages: {} sent, {} received", snap.compressedUp, snap.compressedDown);
        log::info("> Compression savings: {} bytes ({}%), avg ratio {} (sent); {} bytes ({}%), avg ratio {} (recv)",
            snap.compUpSavedBytes, snap.compUpSavedPercent, snap.compUpAvgRatio,
            snap.compDownSavedBytes, snap.compDownSavedPercent, snap.compDownAvgRatio
        );

        if (snap.upReliable || snap.downReliable) {
            log::info("> Reliable messages: {} sent, {} received; retransmissions: {}, unacked: {}, unknown acks: {}",
                snap.upReliable, snap.downReliable, snap.upRetransmissions, snap.upUnacked, snap.downUnknownAcks
            );
            log::info("> Ack delay: min {}, max {}, avg {}",
                snap.minAckDelay.toString(), snap.maxAckDelay.toString(), snap.avgAckDelay.toString()
            );
        }
    };

    log::info("=== Central connection stats ===");
    describe(m_centralConn->statSnapshotFull());
    log::info("==== Game connection stats =====");
    describe(m_gameConn->statSnapshotFull());
    log::info("================================");
}

void NetworkManagerImpl::simulateConnectionDrop() {
    m_centralConn->simulateConnectionDrop();
}

std::optional<uint8_t> NetworkManagerImpl::getPreferredServer() {
    auto info = this->connInfo();
    if (!info) return std::nullopt;
    auto& servers = info->m_gameServers;

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
    auto info = this->connInfo();
    if (!info) {
        return {};
    }

    return asp::iter::values(info->m_gameServers).collect();
}

bool NetworkManagerImpl::isConnected() const {
    auto info = this->connInfo();
    return info && info->m_established;
}

bool NetworkManagerImpl::isGameConnected() const {
    auto info = this->connInfo();
    return info && info->m_gameEstablished && m_gameConn->state() == qn::ConnectionState::Connected;
}

Duration NetworkManagerImpl::getGamePing() {
    return m_gameConn ? m_gameConn->getLatency() : Duration::zero();
}

Duration NetworkManagerImpl::getCentralPing() {
    return m_centralConn ? m_centralConn->getLatency() : Duration::zero();
}

std::string NetworkManagerImpl::getCentralIdent() {
    auto info = this->connInfo();
    if (!info || info->m_centralUrl.empty()) return "";

    auto hash = qn::blake3Hash(info->m_centralUrl).toString();
    hash.resize(16);
    return hash;
}

void NetworkManagerImpl::clearAllUTokens() {
    ValueManager::get().eraseWithPrefix("auth.last-utoken.");
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
    auto info = this->connInfo();
    return info ? info->m_gameTickrate : 0;
}

std::vector<UserRole> NetworkManagerImpl::getAllRoles() {
    auto info = this->connInfo();
    return info ? info->m_allRoles : std::vector<UserRole>{};
}

std::vector<UserRole> NetworkManagerImpl::getUserRoles() {
    auto info = this->connInfo();
    return info ? info->m_userRoles : std::vector<UserRole>{};
}

std::vector<uint8_t> NetworkManagerImpl::getUserRoleIds() {
    auto info = this->connInfo();
    return info ? info->m_userRoleIds : std::vector<uint8_t>{};
}

std::optional<UserRole> NetworkManagerImpl::getUserHighestRole() {
    auto info = this->connInfo();
    if (!info) return std::nullopt;

    // we assume that roles are sorted by priority, so the first one is the highest
    auto& roles = info->m_userRoles;
    return roles.empty() ? std::nullopt : std::make_optional(roles.front());
}

std::optional<UserRole> NetworkManagerImpl::findRole(uint8_t roleId) {
    auto info = this->connInfo();
    if (!info) return std::nullopt;

    for (auto& role : info->m_allRoles) {
        if (role.id == roleId) {
            return role;
        }
    }

    return std::nullopt;

}

std::optional<UserRole> NetworkManagerImpl::findRole(std::string_view roleId) {
    auto info = this->connInfo();
    if (!info) return std::nullopt;

    for (auto& role : info->m_allRoles) {
        if (role.stringId == roleId) {
            return role;
        }
    }

    return std::nullopt;
}

bool NetworkManagerImpl::isModerator() {
    return this->getUserPermissions().isModerator;
}

bool NetworkManagerImpl::isAuthorizedModerator() {
    auto info = this->connInfo();
    return info && info->m_authorizedModerator;
}

UserPermissions NetworkManagerImpl::getUserPermissions() {
    auto info = this->connInfo();
    return info ? info->m_perms : UserPermissions{};
}

PunishReasons NetworkManagerImpl::getModPunishReasons() {
    auto info = this->connInfo();
    return info ? info->m_punishReasons : PunishReasons{};
}

std::optional<SpecialUserData> NetworkManagerImpl::getOwnSpecialData() {
    auto info = this->connInfo();
    if (info && (info->m_userRoleIds.size() || info->m_nameColor)) {
        SpecialUserData data{};
        data.roleIds = info->m_userRoleIds;
        data.nameColor = info->m_nameColor;
        return data;
    }

    return std::nullopt;
}

void NetworkManagerImpl::invalidateIcons() {
    auto info = this->connInfo();
    if (info) {
        info->m_sentIcons = false;
    }
}

void NetworkManagerImpl::invalidateFriendList() {
    auto info = this->connInfo();
    if (info) {
        info->m_sentFriendList = false;
    }
}

void NetworkManagerImpl::markAuthorizedModerator() {
    auto info = this->connInfo();
    if (info) {
        info->m_authorizedModerator = true;
    }
}

std::optional<FeaturedLevelMeta> NetworkManagerImpl::getFeaturedLevel() {
    auto info = this->connInfo();
    return info ? info->m_featuredLevel : std::nullopt;
}

bool NetworkManagerImpl::hasViewedFeaturedLevel() {
    return this->getLastFeaturedLevelId() == this->getFeaturedLevel().value_or(FeaturedLevelMeta{}).levelId;
}

void NetworkManagerImpl::setViewedFeaturedLevel() {
    this->setLastFeaturedLevelId(this->getFeaturedLevel().value_or(FeaturedLevelMeta{}).levelId);
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

int32_t NetworkManagerImpl::getLastFeaturedLevelId() {
    return globed::value<int32_t>(fmt::format("core.last-featured-id.{}", this->getCentralIdent())).value_or(0);
}

void NetworkManagerImpl::setLastFeaturedLevelId(int32_t id) {
    ValueManager::get().set(fmt::format("core.last-featured-id.{}", this->getCentralIdent()), id);
}

void NetworkManagerImpl::onCentralStateChanged(qn::ConnectionState state) {
    log::info("central connection state: {}", connectionStateToStr(state));
    m_workerNotify.notifyOne();

    if (state == qn::ConnectionState::Disconnected) {
        // reset some other singletons
        RoomManager::get().reset();
    }
}

void NetworkManagerImpl::onGameStateChanged(qn::ConnectionState state) {
    log::info("game connection state: {}", connectionStateToStr(state));
    {
        auto info = this->connInfo();
        if (info && state == qn::ConnectionState::Disconnected) {
            info->m_gameEstablished = false;
        }
    }
    m_gameWorkerNotify.notifyOne();
}

void NetworkManagerImpl::abortConnection(std::string reason, bool silent) {
    log::warn("aborting connection to central server: {}", reason);
    *m_abortCause.lock() = std::make_pair(std::move(reason), silent);
    m_centralConn->disconnect();
    m_gameConn->disconnect();
}

void NetworkManagerImpl::logQunetMessage(qn::log::Level level, const std::string& msg) {
    if (!m_centralLogger && !m_gameLogger) return;

    // auto content = fmt::format("[Qunet] [{}] {}", qn::log::levelToString(level), msg);
    // if (m_centralLogger) m_centralLogger->sendTextLog(content);
    // if (m_gameLogger) m_gameLogger->sendTextLog(content);
}

void NetworkManagerImpl::logArcMessage(arc::LogLevel level, const std::string& msg) {
    if (!m_centralLogger && !m_gameLogger) return;

    // auto content = fmt::format("[Arc] [{}] {}", arc::levelToString(level), msg);
    // if (m_centralLogger) m_centralLogger->sendTextLog(content);
    // if (m_gameLogger) m_gameLogger->sendTextLog(content);
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

void NetworkManagerImpl::handleLoginFailed(schema::main::LoginFailedReason reason) {
    using enum schema::main::LoginFailedReason;

    auto info = this->connInfo();
    info->m_established = false;
    info->m_authenticating = false;
    info.unlock();

    switch (reason) {
        case INVALID_USER_TOKEN: {
            log::warn("Login failed: invalid user token, clearing token and trying to re-auth");
            this->clearUToken();
            m_workerNotify.notifyOne();
        } break;

        case INVALID_ARGON_TOKEN: {
            log::warn("Login failed: invalid Argon token, clearing token and trying to re-auth");
            argon::clearToken();
            m_workerNotify.notifyOne();
        } break;

        case ARGON_NOT_SUPPORTED: {
            log::warn("Login failed: Argon is not supported by the server, falling back to plain login");
            info.relock();
            info->m_knownArgonUrl.clear();
            m_workerNotify.notifyOne();
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

void NetworkManagerImpl::handleSuccessfulLogin(LockedConnInfo& info) {
    m_workerNotify.notifyOne();
}

// Central messages

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

void NetworkManagerImpl::sendUpdatePinnedLevel(uint64_t sid) {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        auto upd = msg.initUpdatePinnedLevel();
        upd.setId(sid);
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

// Game server messages

void NetworkManagerImpl::sendGameLoginRequest(SessionId id, bool platformer, bool editorCollab) {
    this->sendToGame([&](GameMessage::Builder& msg) {
        auto login = msg.initLogin();
        login.setAccountId(g_argonData.accountId);
        login.setToken(this->getUToken().value_or(""));
        data::encode(PlayerIconData::getOwn(), login.initIcons());
        gatherUserSettings(login.initSettings());

        if (id.asU64() != 0) {
            login.setSessionId(id);
            login.setPasscode(RoomManager::get().getPasscode());
            login.setPlatformer(platformer);
            login.setEditorCollab(editorCollab);
        }
    });
}

void NetworkManagerImpl::sendGameJoinRequest(SessionId id, bool platformer, bool editorCollab) {
    this->sendToGame([&](GameMessage::Builder& msg) {
        auto join = msg.initJoinSession();
        join.setSessionId(id);
        join.setPlatformer(platformer);
        join.setEditorCollab(editorCollab);
        join.setPasscode(RoomManager::get().getPasscode());
    });
}

void NetworkManagerImpl::sendPlayerState(const PlayerState& state, const std::vector<int>& dataRequests, CCPoint cameraCenter, float cameraRadius) {
    auto info = this->connInfo();
    if (!info || !info->m_gameEstablished || m_gameConn->state() != qn::ConnectionState::Connected) {
        log::warn("Cannot send player state, not connected to game server");
        return;
    }

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
        for (size_t i = 0; i < std::min<size_t>(64, info->m_gameEventQueue.size()); i++) {
            auto& event = info->m_gameEventQueue.front();
            if (auto err = event.encode(eventEncoder).err()) {
                log::warn("Failed to encode event: {}", *err);
            }

            info->m_gameEventQueue.pop();
        }

        auto eventData = std::move(eventEncoder).intoVector();
        playerData.setEventData(kj::ArrayPtr{eventData.data(), eventData.size()});
    }, !info->m_gameEventQueue.empty());
}

void NetworkManagerImpl::queueLevelScript(const std::vector<EmbeddedScript>& scripts) {
    auto info = this->connInfo();
    if (!info) return;

    if (info->m_gameEstablished) {
        info->m_queuedScripts.clear();
        this->sendLevelScript(scripts);
    } else {
        info->m_queuedScripts = scripts;
    }
}

void NetworkManagerImpl::sendLevelScript(const std::vector<EmbeddedScript>& scripts) {
    this->sendToGame([&](GameMessage::Builder& msg) {
        data::encode(scripts, msg.initSendLevelScript());
    }, true, true);
}

void NetworkManagerImpl::queueGameEvent(OutEvent&& event) {
    auto info = this->connInfo();
    if (!info) return;

    info->m_gameEventQueue.push(std::move(event));
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

// Messages for both servers

void NetworkManagerImpl::sendJoinSession(SessionId id, int author, bool platformer, bool editorCollab) {
    auto info = this->connInfo();
    if (!info) {
        log::error("Cannot send join session, not connected");
        return;
    }

    log::debug("Joining session with ID {} (editorcollab: {})", id.asU64(), editorCollab);

    if (!editorCollab) {
        this->sendToCentral([&](CentralMessage::Builder& msg) {
            auto joinSession = msg.initJoinSession();
            joinSession.setSessionId(id);
            joinSession.setAuthorId(author);
        });
    }

    // find the game server
    // TODO: editor collab levels are currently hardcoded to use server ID 0
    uint8_t serverId = editorCollab ? 0 : id.serverId();

    for (auto& srv : info->m_gameServers) {
        if (srv.second.id == serverId) {
            this->joinSessionWith(info, srv.second.url, id, platformer, editorCollab);
            return;
        }
    }

    // not found!
    log::error("Tried joining an invalid session, game server with ID {} does not exist", static_cast<int>(serverId));
    toastError("Globed: join failed, no server with ID {}", serverId);
}

void NetworkManagerImpl::joinSessionWith(LockedConnInfo& info, std::string_view serverUrl, SessionId id, bool platformer, bool editorCollab) {
    GameServerJoinRequest req;
    req.url = serverUrl;
    req.id = id;
    req.platformer = platformer;
    req.editorCollab = editorCollab;
    (void) m_gameServerJoinTx->trySend(std::move(req));
}

void NetworkManagerImpl::sendLeaveSession() {
    this->sendToCentral([&](CentralMessage::Builder& msg) {
        msg.initLeaveSession();
    });

    this->sendToGame([&](GameMessage::Builder& msg) {
        msg.initLeaveSession();
    });
}

Result<> NetworkManagerImpl::onCentralDataReceived(CentralMessage::Reader& msg) {
    using enum CentralMessage::Which;

    switch (msg.which()) {
        case LOGIN_OK: {
            auto info = this->connInfo();
            auto loginOk = msg.getLoginOk();

            if (loginOk.hasServers()) {
                auto servers = loginOk.getServers();
                updateServers(*info, servers);
            }

            auto res = data::decodeOpt<msg::CentralLoginOkMessage>(loginOk);
            if (!res) {
                this->abortConnection("invalid CentralLoginOkMessage");
                return Err("invalid CentralLoginOkMessage");
            }

            auto msg = std::move(res).value();

            info->m_allRoles = msg.allRoles;
            info->m_userRoleIds = msg.userData.roleIds;
            info->m_established = true;
            info->m_perms = msg.userData.permissions;
            info->m_nameColor = msg.userData.nameColor;
            info->m_featuredLevel = msg.featuredLevel;

            for (auto id : info->m_userRoleIds) {
                if (id < msg.allRoles.size()) {
                    info->m_userRoles.push_back(msg.allRoles[id]);
                } else {
                    log::warn("Unknown role ID in CentralLoginOkMessage: {}", id);
                }
            }

            this->handleSuccessfulLogin(info);
            info.unlock();

            if (!msg.userData.newToken.empty()) {
                this->setUToken(msg.userData.newToken);
            }

            this->invokeListeners(std::move(msg));
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

            // store the argon url and try to re-auth
            {
                auto info = this->connInfo();
                info->m_knownArgonUrl = loginRequired.getArgonUrl();
                info->m_authenticating = false;
            }

            this->clearUToken();
            m_workerNotify.notifyOne();
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
            auto serversChanged = msg.getServersChanged();
            auto servers = serversChanged.getServers();

            auto info = this->connInfo();
            updateServers(*info, servers);
        } break;

        case CentralMessage::USER_DATA_CHANGED: {
            auto res = data::decodeOpt<msg::UserDataChangedMessage>(msg.getUserDataChanged());
            if (!res) {
                log::warn("Server sent invalid UserDataChangedMessage");
                break;
            }

            auto msg = std::move(*res);
            auto& ud = msg.userData;
            auto info = this->connInfo();

            info->m_perms = ud.permissions;
            info->m_nameColor = ud.nameColor;
            info->m_userRoleIds = ud.roleIds;

            info->m_userRoles.clear();
            for (auto id : ud.roleIds) {
                if (id < info->m_allRoles.size()) {
                    info->m_userRoles.push_back(info->m_allRoles[id]);
                } else {
                    log::warn("Unknown role ID: {}", id);
                }
            }
            info.unlock();

            if (!ud.newToken.empty()) {
                this->setUToken(ud.newToken);
            }

            this->invokeListeners(std::move(msg));
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

        case CentralMessage::PINNED_LEVEL_UPDATED: {
            auto m = msg.getPinnedLevelUpdated();
            this->invokeListeners(msg::PinnedLevelUpdatedMessage { m.getId() });
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

        case CentralMessage::ROOM_WARP: {
            auto roomWarp = msg.getRoomWarp();
            auto sessionId = SessionId{roomWarp.getSession()};

            this->invokeListeners(msg::RoomWarpMessage{sessionId});
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
            this->invokeListeners(msg::WarnMessage{msg.getWarn().getMessage()});
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
            this->connInfo()->m_featuredLevel = out.meta;

            this->invokeListeners(std::move(out));
        } break;

        case CentralMessage::FEATURED_LIST: {
            this->invokeListeners(data::decodeUnchecked<msg::FeaturedListMessage>(msg.getFeaturedList()));
        } break;

        //

        case CentralMessage::ADMIN_RESULT: {
            auto adminResult = msg.getAdminResult();

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

            // admin punishment reasons is de facto the "admin login successful" message,
            // so after receiving it we can mark ourselves as an authorized moderator
            this->markAuthorizedModerator();

            this->connInfo()->m_punishReasons = m.reasons;
            this->invokeListeners(std::move(m));
        } break;

        default: {
            return Err("Received unknown message type: {}", std::to_underlying(msg.which()));
        } break;
    }

    return Ok();
}

Result<> NetworkManagerImpl::onGameDataReceived(GameMessage::Reader& msg) {
    using enum GameMessage::Which;

    switch (msg.which()) {
        case LOGIN_OK: {
            auto lock = m_connInfo.lock();
            auto& connInfo = **lock;

            auto loginOk = msg.getLoginOk();

            connInfo.m_gameEstablished = true;
            connInfo.m_gameTickrate = loginOk.getTickrate();
            log::debug("Successfully logged in to game server");
        } break;

        case LOGIN_FAILED: {
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

            m_gameConn->disconnect();
        } break;

        case JOIN_SESSION_OK: {
            // TODO: post listener message
        } break;

        case JOIN_SESSION_FAILED: {
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

        case LEVEL_DATA: {
            this->invokeListeners(data::decodeUnchecked<msg::LevelDataMessage>(msg.getLevelData()));
        } break;

        case SCRIPT_LOGS: {
            this->invokeListeners(data::decodeUnchecked<msg::ScriptLogsMessage>(msg.getScriptLogs()));
        } break;

        case VOICE_BROADCAST: {
            this->invokeListeners(data::decodeUnchecked<msg::VoiceBroadcastMessage>(msg.getVoiceBroadcast()));
        } break;

        case QUICK_CHAT_BROADCAST: {
            this->invokeListeners(data::decodeUnchecked<msg::QuickChatBroadcastMessage>(msg.getQuickChatBroadcast()));
        } break;

        case CHAT_NOT_PERMITTED: {
            this->invokeListeners(msg::ChatNotPermittedMessage{});
        } break;

        case KICKED: {
            // TODO
        } break;

        default: {
            return Err("Received unknown message type: {}", std::to_underlying(msg.which()));
        } break;
    }

    return Ok();
}

}
