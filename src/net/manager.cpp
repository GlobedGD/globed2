#include "manager.hpp"

#include "address.hpp"
#include "listener.hpp"
#include "game_socket.hpp"

#include <asp/sync.hpp>
#include <asp/thread.hpp>

#include <data/packets/all.hpp>
#include <defs/minimal_geode.hpp>
#include <managers/account.hpp>
#include <managers/admin.hpp>
#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <managers/profile_cache.hpp>
#include <managers/settings.hpp>
#include <managers/room.hpp>
#include <managers/role.hpp>
#include <util/cocos.hpp>
#include <util/time.hpp>
#include <util/net.hpp>

using namespace asp::sync;
using namespace geode::prelude;
using ConnectionState = NetworkManager::ConnectionState;

static constexpr uint16_t PROTOCOL_VERSION = 6;

struct AtomicConnectionState {
    AtomicInt inner;

    AtomicConnectionState& operator=(ConnectionState state) {
        inner.store(static_cast<int>(state));
        return *this;
    }

    operator ConnectionState() {
        return static_cast<ConnectionState>(inner.load());
    }
};

class NetworkManager::Impl {
protected:
    friend class NetworkManager;

    static constexpr int BUILTIN_LISTENER_PRIORITY = -10000000;

    struct TaskPingServers {};
    struct TaskSendPacket {
        std::shared_ptr<Packet> packet;
    };

    using Task = std::variant<TaskPingServers, TaskSendPacket>;

    // yes, really
    AtomicConnectionState state;
    GameSocket socket;
    asp::Thread<NetworkManager::Impl*> threadRecv, threadMain;
    asp::Channel<Task> taskQueue;

    // Note that we intentionally don't use Ref here,
    // as we use the destructor to know if the object owning the listener has been destroyed.
    asp::Mutex<std::unordered_map<packetid_t, std::vector<PacketListener*>>> listeners;
    asp::Mutex<std::unordered_map<packetid_t, util::time::system_time_point>> suppressed;

    // these fields are only used by us and in a safe manner, so they don't need a mutex
    NetworkAddress connectedAddress;
    std::string connectedServerId;
    util::time::time_point lastReceivedPacket;
    util::time::time_point lastSentKeepalive;
    util::time::time_point lastTcpExchange;

    AtomicBool suspended;
    AtomicBool standalone;
    AtomicBool recovering;
    AtomicU8 recoverAttempt;
    AtomicBool ignoreProtocolMismatch;
    AtomicU32 secretKey;
    AtomicU32 serverTps;

    Impl() {
        // initialize winsock
        util::net::initialize();

        this->setupGlobalListeners();

        // start up the threads

        threadRecv.setLoopFunction(&NetworkManager::Impl::threadRecvFunc);
        threadRecv.setStartFunction([] { geode::utils::thread::setName("Network Thread (in)"); });
        threadRecv.start(this);

        threadMain.setLoopFunction(&NetworkManager::Impl::threadMainFunc);
        threadMain.setStartFunction([] { geode::utils::thread::setName("Network Thread (out)"); });
        threadMain.start(this);
    }

    ~Impl() {
        // remove all listeners
        this->removeAllListeners();

        log::debug("waiting for network threads to terminate..");
        threadRecv.stopAndWait();
        threadMain.stopAndWait();

        if (state != ConnectionState::Disconnected) {
            log::debug("disconnecting from the server..");
            try {
                this->disconnect(false, true);
            } catch (const std::exception& e) {
                log::warn("error trying to disconnect: {}", e.what());
            }
        }

        util::net::cleanup();

        log::info("goodbye from networkmanager!");
    }

    // connection and tasks
    Result<> connect(const NetworkAddress& address, const std::string_view serverId, bool standalone) {
        // if we are already connected, disconnect first
        if (state == ConnectionState::Established) {
            this->disconnect(false, false);
        }

        if (state != ConnectionState::Disconnected) {
            return Err("already trying to connect");
        }

        this->standalone = standalone;

        lastReceivedPacket = util::time::now();
        lastSentKeepalive = util::time::now();
        lastTcpExchange = util::time::now();

        if (!standalone) {
            GLOBED_REQUIRE_SAFE(!GlobedAccountManager::get().authToken.lock()->empty(), "attempting to connect with no authtoken set in account manager")
        }

        connectedAddress = address;
        connectedServerId = std::string(serverId);
        recovering = false;
        recoverAttempt = 0;

        state = ConnectionState::TcpConnecting;

        // actual connection is deferred - the network thread does DNS resolution and TCP connection.

        return Ok();
    }

    void disconnect(bool quiet = false, bool noclear = false) {
        ConnectionState prevState = state;

        state = ConnectionState::Disconnected;
        standalone = false;
        recovering = false;
        recoverAttempt = 0;

        if (!quiet && prevState == ConnectionState::Established) {
            // send it directly instead of pushing to the queue
            (void) socket.sendPacket(DisconnectPacket::create());
        }

        socket.disconnect();

        // singletons could have been destructed before NetworkManager, so this could be UB. Additionally will break autoconnect.
        if (!noclear) {
            RoomManager::get().setGlobal();
            GameServerManager::get().clearActive();
            AdminManager::get().deauthorize();
        }
    }

    void disconnectWithMessage(const std::string_view message, bool quiet = true) {
        ErrorQueues::get().error(fmt::format("You have been disconnected from the active server.\n\nReason: <cy>{}</c>", message));
        this->disconnect(quiet);
    }

    void onConnectionError(const std::string_view reason) {
        if (state != ConnectionState::Established) {
            log::warn("unable to disconnect, not established");
            return;
        }

        ErrorQueues::get().warn(reason);
    }

    void send(std::shared_ptr<Packet> packet) {
        taskQueue.push(TaskSendPacket {
            .packet = std::move(packet)
        });
    }

    void pingServers() {
        taskQueue.push(TaskPingServers {});
    }

    ConnectionState getConnectionState() {
        return state;
    }

    bool established() {
        return state == ConnectionState::Established;
    }

    /* listeners */

    std::string formatListenerKey(packetid_t id) {
        return util::cocos::spr(fmt::format("packet-listener-{}", id));
    }

    void addListener(cocos2d::CCNode* target, PacketListener* listener) {
        if (target) {
            target->setUserObject(this->formatListenerKey(listener->packetId), listener);
        } else {
            // if target is nullptr we leak the listener,
            // essentially saying it should live for the entire duration of the program.
            listener->retain();
        }

        this->registerPacketListener(listener->packetId, listener);
    }

    void registerPacketListener(packetid_t packet, PacketListener* listener) {
#ifdef GLOBED_DEBUG
        log::debug("Registering listener (id {}) for {}", packet, listener->owner);
#endif

        auto ls = listeners.lock();
        auto& lsm = (*ls)[packet];

        // is it already added?
        auto it = std::find(lsm.begin(), lsm.end(), listener);
        if (it != lsm.end()) {
            return;
        }

        // otherwise, add it
        lsm.push_back(listener);

        // sort all elements by priority descending (higher = runs earlier)
        std::sort(lsm.begin(), lsm.end(), [](PacketListener* a, PacketListener* b) -> bool {
            return a->priority > b->priority;
        });
    }

    void removeListener(cocos2d::CCNode* target, packetid_t id) {
        auto key = util::cocos::spr(fmt::format("packet-listener-{}", id));

        target->setUserObject(key, nullptr);

        // the listener will unregister itself in the destructor.
    }

    void unregisterPacketListener(packetid_t packet, PacketListener* listener, bool suppressUnhandled) {
#ifdef GLOBED_DEBUG
        // note: is it safe to access listener->owner here?
        // at the time of user object destruction, i believe the node is still valid,
        // but we are inside of ~CCNode(), which means main vtable is set to CCNode vtable,
        // and therefore this would always print 'class cocos2d::CCNode' instead of the real class.
        log::debug("Unregistering listener (id {}) for {}", packet, listener->owner);
#endif

        auto ls = listeners.lock();
        auto& lsm = (*ls)[packet];

        auto it = std::find(lsm.begin(), lsm.end(), listener);
        if (it != lsm.end()) {
            lsm.erase(it);
        }

        if (suppressUnhandled) {
            this->suppressUnhandledUntil(packet, util::time::systemNow() + util::time::seconds(3));
        }
    }

    void suppressUnhandledUntil(packetid_t id, util::time::system_time_point point) {
        (*suppressed.lock())[id] = point;
    }

    void removeAllListeners() {
        listeners.lock()->clear();
    }

    void setupGlobalListeners() {
        // Connection packets
        addGlobalListenerSync<CryptoHandshakeResponsePacket>([this](auto packet) {
            this->onCryptoHandshakeResponse(std::move(packet));
        });

        addGlobalListener<KeepaliveResponsePacket>([](auto packet) {
            GameServerManager::get().finishKeepalive(packet->playerCount);
        });

        addGlobalListener<KeepaliveTCPResponsePacket>([](auto) {});

        addGlobalListener<ServerDisconnectPacket>([this](auto packet) {
            this->disconnectWithMessage(packet->message);
        });

        addGlobalListener<LoggedInPacket>([this](auto packet) {
            this->onLoggedIn(std::move(packet));
        });

        addGlobalListener<LoginFailedPacket>([this](auto packet) {
            ErrorQueues::get().error(fmt::format("<cr>Authentication failed!</c> The server rejected the login attempt.\n\nReason: <cy>{}</c>", packet->message));
            GlobedAccountManager::get().authToken.lock()->clear();
            this->disconnect(true);
        });

        addGlobalListener<ClaimThreadFailedPacket>([this](auto packet) {
            this->disconnectWithMessage("failed to claim udp thread");
        });
    }

    void onCryptoHandshakeResponse(std::shared_ptr<CryptoHandshakeResponsePacket> packet) {
        log::debug("handshake successful, logging in");
        auto key = packet->data.key;

        socket.cryptoBox->setPeerKey(key.data());
        auto& am = GlobedAccountManager::get();
        std::string authtoken;

        if (!standalone) {
            authtoken = *am.authToken.lock();
        }

        auto& pcm = ProfileCacheManager::get();
        pcm.setOwnDataAuto();
        pcm.pendingChanges = false;

        auto& settings = GlobedSettings::get();

        if (settings.globed.fragmentationLimit == 0) {
            settings.globed.fragmentationLimit = 65000;
        }

        auto gddata = am.gdData.lock();
        auto pkt = LoginPacket::create(
            gddata->accountId,
            gddata->userId,
            gddata->accountName,
            authtoken,
            pcm.getOwnData(),
            settings.globed.fragmentationLimit,
            util::net::loginPlatformString()
        );

        this->send(pkt);
    }

    void onLoggedIn(std::shared_ptr<LoggedInPacket> packet) {
        log::info("Successfully logged into the server!");
        serverTps = packet->tps;
        secretKey = packet->secretKey;
        state = ConnectionState::Established;

        if (recovering) {
            recovering = false;
            recoverAttempt = 0;

            log::info("connection recovered successfully!");
            ErrorQueues::get().success("Connection recovered");
        }

        // these are not thread-safe, so delay it
        Loader::get()->queueInMainThread([specialUserData = std::move(packet->specialUserData), allRoles = std::move(packet->allRoles)] {
            auto& pcm = ProfileCacheManager::get();
            pcm.setOwnSpecialData(specialUserData);

            RoomManager::get().setGlobal();
            RoleManager::get().setAllRoles(allRoles);
        });

        // claim the tcp thread to allow udp packets through
        this->send(ClaimThreadPacket::create(this->secretKey));

        // try to login as an admin if we can
        auto& am = GlobedAccountManager::get();
        if (am.hasAdminPassword()) {
            auto password = am.getAdminPassword();
            if (password.has_value()) {
                this->send(AdminAuthPacket::create(password.value()));
            }
        }
    }

    void addGlobalListener(packetid_t id, PacketCallback&& callback, bool mainThread = false) {
        auto listener = PacketListener::create(id, std::move(callback), nullptr, BUILTIN_LISTENER_PRIORITY, mainThread, false);
        this->addListener(nullptr, listener);
    }

    template <HasPacketID Pty>
    void addGlobalListener(PacketCallbackSpecific<Pty>&& callback, bool mainThread = false) {
        this->addGlobalListener(Pty::PACKET_ID, [callback = std::move(callback)](std::shared_ptr<Packet> pkt) {
            callback(std::static_pointer_cast<Pty>(pkt));
        }, mainThread);
    }

    // Same as `addGlobalListener(callback, true)`
    template <HasPacketID Pty>
    void addGlobalListenerSync(PacketCallbackSpecific<Pty>&& callback) {
        this->addGlobalListener<Pty>(std::move(callback), true);
    }

    /* misc */

    void togglePacketLogging(bool enabled) {
        socket.togglePacketLogging(enabled);
    }

    void setIgnoreProtocolMismatch(bool state) {
        ignoreProtocolMismatch = state;
    }

    uint16_t getUsedProtocol() {
        // 0xffff is a special value that the server doesn't check
        return ignoreProtocolMismatch ? 0xffff : PROTOCOL_VERSION;
    }

    uint32_t getServerTps() {
        return established() ? serverTps.load() : 0;
    }

    bool isStandalone() {
        return standalone;
    }

    /* suspension */

    void suspend() {
        suspended = true;
    }

    void resume() {
        suspended = false;
    }

    /* worker threads */

    void threadRecvFunc() {
        if (this->suspended || state == ConnectionState::TcpConnecting) {
            std::this_thread::sleep_for(util::time::millis(100));
            return;
        }

        // TODO: keepalives?
        auto packet_ = socket.recvPacket(100);
        if (packet_.isErr()) {
            auto error = std::move(packet_.unwrapErr());
            if (error != "timed out") {
                this->onConnectionError(error);
            }

            return;
        }

        // this is awesome
        auto packet__ = packet_.unwrap();
        auto packet = std::move(packet__.packet);
        bool fromServer = packet__.fromConnected;

        packetid_t id = packet->getPacketId();

        if (id == PingResponsePacket::PACKET_ID) {
            this->handlePingResponse(std::move(packet));
            return;
        }

        // if it's not a ping packet, and it's NOT from the currently connected server, we reject it
        if (!fromServer) {
            return;
        }

        lastReceivedPacket = util::time::now();

        this->callListener(std::move(packet));
    }

    void callListener(std::shared_ptr<Packet>&& packet) {
        packetid_t packetId = packet->getPacketId();

        auto ls = listeners.lock();
        auto& lsm = (*ls)[packetId];

        if (lsm.empty()) {
            this->handleUnhandledPacket(packetId);
            return;
        }
        // in case any of the listeners try to destruct themselves (i.e. destroying thir parent),
        // we should retain and release them after unlocking `listeners`, to prevent deadlocks.

        for (auto* listener : lsm) {
            listener->retain();
        }

        // invoke callbacks
        for (auto* listener : lsm) {
            listener->invokeCallback(packet);

            if (listener->isFinal) break;
        }

        std::vector<PacketListener*> lsmCopy(lsm);
        ls.unlock();

        for (auto* listener : lsmCopy) {
            listener->release();
        }
    }

    void handleUnhandledPacket(packetid_t packetId) {
        // if suppressed, do nothing
        auto suppressed_ = suppressed.lock();

        if (suppressed_->contains(packetId) && util::time::systemNow() > suppressed_->at(packetId)) {
            suppressed_->erase(packetId);
        }

        // else show a warning
        if (!suppressed_->contains(packetId)) {
            ErrorQueues::get().debugWarn(fmt::format("Unhandled packet: {}", packetId));
        }
    }

    void handlePingResponse(std::shared_ptr<Packet>&& packet) {
        if (auto* pingr = packet->tryDowncast<PingResponsePacket>()) {
            GameServerManager::get().finishPing(pingr->id, pingr->playerCount);
        }
    }

    void threadMainFunc() {
        if (this->suspended) {
            std::this_thread::sleep_for(util::time::millis(100));
            return;
        }

        if (state == ConnectionState::TcpConnecting && !recovering) {
            // initiate TCP connection
            auto result = socket.connect(connectedAddress, recovering);

            if (!result) {
                this->disconnect(true);
                ErrorQueues::get().error(fmt::format("TCP connection failed: <cy>{}</c>", result.unwrapErr()));
                return;
            } else {
                log::debug("tcp connection successful, sending the handshake");
                state = ConnectionState::Authenticating;

                socket.createBox();
                GameServerManager::get().setActive(connectedServerId);

                uint16_t proto = this->getUsedProtocol();

                this->send(CryptoHandshakeStartPacket::create(
                    proto,
                    CryptoPublicKey(socket.cryptoBox->extractPublicKey())
                ));
            }
        } else if (state == ConnectionState::TcpConnecting && recovering) {
            log::debug("recovery attempt {}", recoverAttempt.load());

            // initiate TCP connection
            auto result = socket.connect(connectedAddress, recovering);

            bool failed = false;

            if (!result) {
                failed = true;
            } else {
                log::debug("tcp connection successful, sending recovery data");
                state = ConnectionState::Authenticating;

                // if we are recovering, next steps are slightly different
                // first send our account ID and secret key (handled on the lowest level by the server)
                auto result = socket.sendRecoveryData(GJAccountManager::get()->m_accountID, secretKey);

                if (!result) {
                    failed = true;
                } else {
                    // the rest is done in a global listener
                    return;
                }
            }

            if (failed) {
                auto attemptNumber = recoverAttempt.load() + 1;
                recoverAttempt = attemptNumber;

                if (attemptNumber > 3) {
                    // give up
                    this->failedRecovery();
                    return;
                }

                // wait for a bit before trying again
                std::this_thread::sleep_for(util::time::seconds(5) * attemptNumber);
                return;
            }
        }

        // detect if the tcp socket has disconnected and start recovery attempt
        if (state == ConnectionState::Established && !socket.isConnected()) {
            ErrorQueues::get().warn("socket has disconnected, attempting to recover the connection..");

            state = ConnectionState::TcpConnecting;
            recovering = true;
            recoverAttempt = 0;
            return;
        }

        if (this->established()) {
            this->maybeSendKeepalive();
        }

        // poll for any incoming packets

        while (auto task_ = taskQueue.popTimeout(util::time::millis(50))) {
            auto task = task_.value();

            if (std::holds_alternative<TaskPingServers>(task)) {
                this->handlePingTask();
            } else if (std::holds_alternative<TaskSendPacket>(task)) {
                this->handleSendPacketTask(std::move(std::get<TaskSendPacket>(task)));
            }
        }

        std::this_thread::yield();
    }

    void maybeSendKeepalive() {
        auto now = util::time::now();

        auto sinceLastPacket = now - lastReceivedPacket;
        auto sinceLastKeepalive = now - lastSentKeepalive;
        auto sinceLastTcpExchane = now - lastTcpExchange;

        if (sinceLastPacket > util::time::seconds(20)) {
            this->disconnectWithMessage("timed out (no response from the server after 20 seconds)");
        } else if (sinceLastPacket > util::time::seconds(10) && sinceLastKeepalive < util::time::seconds(3)) {
            // send a keepalive
            this->send(KeepalivePacket::create());
            lastSentKeepalive = now;
            GameServerManager::get().startKeepalive();
        }

        // send a tcp keepalive to keep the nat hole open
        if (sinceLastTcpExchane > util::time::seconds(60)) {
            this->send(KeepaliveTCPPacket::create());
        }
    }

    void failedRecovery() {
        recovering = false;
        recoverAttempt = 0;
        state = ConnectionState::Disconnected;

        ErrorQueues::get().error("Connection to the server was lost. Failed to reconnect after 3 attempts.");
    }

    void handlePingTask() {
        auto& gsm = GameServerManager::get();
        auto active = gsm.getActiveId();

        for (auto& [serverId, server] : gsm.getAllServers()) {
            if (serverId == active) continue;

            NetworkAddress addr(server.address);
#ifdef GLOBED_DEBUG
            auto addrString = addr.resolveToString().value_or("<unresolved>");
            log::debug("sending ping to {}", addrString);
#endif

            auto pingId = gsm.startPing(serverId);
            auto result = socket.sendPacketTo(PingPacket::create(pingId), addr);

            if (result.isErr()) {
                log::debug("failed to send ping: {}", result.unwrapErr());
                ErrorQueues::get().warn(result.unwrapErr());
            }
        }
    }

    void handleSendPacketTask(TaskSendPacket task) {
        if (task.packet->getUseTcp()) {
            lastTcpExchange = util::time::now();
        }

        try {
            auto result = socket.sendPacket(task.packet);
            if (!result) {
                auto error = result.unwrapErr();
                log::debug("failed to send packet {}: {}", task.packet->getPacketId(), error);
                this->onConnectionError(error);
                return;
            }
        } catch (const std::exception& e) {
            this->onConnectionError(e.what());
        }
    }
};

/* NetworkManager (public API) */

NetworkManager::NetworkManager() : impl(new NetworkManager::Impl()) {}

NetworkManager::~NetworkManager() {
    delete impl;
}

Result<> NetworkManager::connect(const NetworkAddress& address, const std::string_view serverId, bool standalone) {
    return impl->connect(address, serverId, standalone);
}

Result<> NetworkManager::connect(const GameServer& gsview) {
    return impl->connect(NetworkAddress(gsview.address), gsview.id, false);
}

Result<> NetworkManager::connectStandalone() {
    auto _server = GameServerManager::get().getServer(GameServerManager::STANDALONE_ID);
    if (!_server.has_value()) {
        return Err(fmt::format("failed to find server by standalone ID"));
    }

    auto server = _server.value();
    return impl->connect(NetworkAddress(server.address), GameServerManager::STANDALONE_ID, true);
}

void NetworkManager::disconnect(bool quiet, bool noclear) {
    impl->disconnect(quiet, noclear);
}

void NetworkManager::disconnectWithMessage(const std::string_view message, bool quiet) {
    impl->disconnectWithMessage(message, quiet);
}

void NetworkManager::send(std::shared_ptr<Packet> packet) {
    impl->send(std::move(packet));
}

void NetworkManager::pingServers() {
    impl->pingServers();
}

void NetworkManager::addListener(cocos2d::CCNode* target, packetid_t id, PacketListener* listener) {
    impl->addListener(target, listener);
}

void NetworkManager::addListener(cocos2d::CCNode* target, packetid_t id, PacketCallback&& callback, int priority, bool mainThread, bool isFinal) {
    auto* listener = PacketListener::create(id, std::move(callback), target, priority, mainThread, isFinal);
    impl->addListener(target, listener);
}

void NetworkManager::removeListener(cocos2d::CCNode* target, packetid_t id) {
    impl->removeListener(target, id);
}

void NetworkManager::removeAllListeners() {
    impl->removeAllListeners();
}

void NetworkManager::togglePacketLogging(bool enabled) {
    impl->togglePacketLogging(enabled);
}

uint16_t NetworkManager::getUsedProtocol() {
    return impl->getUsedProtocol();
}

uint32_t NetworkManager::getServerTps() {
    return impl->getServerTps();
}

bool NetworkManager::standalone() {
    return impl->isStandalone();
}

ConnectionState NetworkManager::getConnectionState() {
    return impl->getConnectionState();
}

bool NetworkManager::established() {
    return impl->established();
}

void NetworkManager::suspend() {
    impl->suspend();
}

void NetworkManager::resume() {
    impl->resume();
}

void NetworkManager::unregisterPacketListener(packetid_t packet, PacketListener* listener, bool suppressUnhandled) {
    impl->unregisterPacketListener(packet, listener, suppressUnhandled);
}
