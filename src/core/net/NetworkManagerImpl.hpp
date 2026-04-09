#pragma once

#include <globed/audio/EncodedAudioFrame.hpp>
#include <globed/core/SessionId.hpp>
#include <globed/core/data/PlayerState.hpp>
#include <globed/core/data/RoomSettings.hpp>
#include <globed/core/data/UserRole.hpp>
#include <globed/core/data/AdminLogs.hpp>
#include <globed/core/data/Event.hpp>
#include <globed/core/data/PunishReasons.hpp>
#include <globed/core/data/FeaturedLevel.hpp>
#include <globed/core/data/UserPermissions.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/util/FunctionQueue.hpp>
#include <modules/scripting/data/EmbeddedScript.hpp>
#include "ConnectionLogger.hpp"

#include <arc/runtime/Runtime.hpp>
#include <arc/sync/mpsc.hpp>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <Geode/utils/function.hpp>
#include "data/generated.hpp"
#include <qunet/Connection.hpp>
#include <qunet/Log.hpp>
#include <Geode/Result.hpp>
#include <Geode/utils/async.hpp>

namespace globed {

using CentralMessage = globed::schema::main::Message;
using GameMessage = globed::schema::game::Message;
using globed::schema::main::RoomOwnerActionType;

struct GameServer {
    uint8_t id;
    std::string stringId;
    std::string url;
    std::string name;
    std::string region;
    uint32_t lastLatency = -1; // millis
    uint32_t avgLatency = -1; // millis
    uint32_t sentPings = 0;
    asp::time::Instant lastPingTime = asp::time::Instant::now();

    /// updates latency, returns if the server latency is considered unstable
    bool updateLatency(uint32_t latency);
};

struct GameServerJoinRequest {
    std::string url;
    SessionId id;
    uint8_t serverId;
    bool platformer;
    bool editorCollab;
    bool triedConnecting = false;
};

struct WorkerState {
    /// Next time to ping game servers
    asp::time::Instant nextGSPing = asp::time::Instant::farFuture();
    arc::mpsc::Sender<std::pair<std::string, qn::PingResult>> pingResultTx;
    arc::mpsc::Receiver<std::pair<std::string, qn::PingResult>> pingResultRx;
    qn::ConnectionState prevCentralState{qn::ConnectionState::Disconnected};

    WorkerState(
        decltype(pingResultTx) tx,
        decltype(pingResultRx) rx
    ) : pingResultTx(std::move(tx)), pingResultRx(std::move(rx)) {}

    void schedulePing(asp::Duration dur) {
        schedulePing(asp::Instant::now() + dur);
    }

    void schedulePing(asp::Instant time) {
        auto now = asp::Instant::now();
        if (nextGSPing <= now) {
            // no scheduled ping in the future
            nextGSPing = time;
        } else {
            // ping scheduled, choose whichever is sooner
            nextGSPing = std::min(nextGSPing, time);
        }
    }
};

struct GameWorkerState {
    qn::ConnectionState prevGameState;
    std::optional<arc::mpsc::Receiver<GameServerJoinRequest>> joinRx;
    std::optional<GameServerJoinRequest> currentReq;
    std::optional<GameServerJoinRequest> lastReq;
};

/// Connection info across a single session. This gets reset on disconnect or stateless reconnect.
struct ConnectionInfo {
    // central server info
    std::string m_centralUrl;
    std::string m_knownArgonUrl;
    std::unordered_map<std::string, GameServer> m_gameServers;
    std::optional<uint8_t> m_serverOverride;
    bool m_gameServersUpdated = true;
    bool m_established = false;
    bool m_authenticating = false;
    asp::time::Instant m_triedAuthAt;

    // game server info
    std::string m_gameServerUrl;
    uint8_t m_gameServerId;
    bool m_gameEstablished = false;
    std::queue<OutEvent> m_gameEventQueue;
    std::vector<EmbeddedScript> m_queuedScripts;
    std::deque<std::pair<uint16_t, asp::Instant>> m_gamePlayerDataReqs;
    std::deque<std::pair<bool, asp::Instant>> m_gameProcessedPackets;
    float m_gameLoss5Secs = 0.f;
    float m_gameLoss1Min = 0.f;
    uint16_t m_gameNextMessageId = 1;

    bool m_sentFriendList = false;
    bool m_sentIcons = true; // icons are sent at login

    // server data send in login ok message
    std::vector<UserRole> m_allRoles;
    std::vector<UserRole> m_userRoles;
    uint32_t m_gameTickrate = 0;
    std::optional<FeaturedLevelMeta> m_featuredLevel;
    std::vector<uint8_t> m_userRoleIds;
    std::optional<MultiColor> m_nameColor;
    UserPermissions m_perms{};
    PunishReasons m_punishReasons{};
    bool m_authorizedModerator;

    void startedAuth() {
        m_authenticating = true;
        m_triedAuthAt = asp::time::Instant::now();
    }

    uint16_t getNextMessageId();
    /// Handles a message from the game server and returns RTT for this packet
    /// Also handles loss calculation
    std::optional<asp::Duration> handleIncomingMessageId(uint16_t id);
    void calculateGameLoss();
};

struct GLOBED_DLL LockedConnInfo {
public:
    LockedConnInfo(asp::MutexGuard<std::optional<ConnectionInfo>, false>&& guard) : _guard(std::move(guard)) {}
    LockedConnInfo(const LockedConnInfo&) = delete;
    LockedConnInfo& operator=(const LockedConnInfo&) = delete;
    LockedConnInfo(LockedConnInfo&&) = default;
    LockedConnInfo& operator=(LockedConnInfo&&) = default;

    operator bool() const {
        return _guard->has_value();
    }

    ConnectionInfo& operator*() {
        this->check();
        return **_guard;
    }

    ConnectionInfo* operator->() {
        this->check();
        return &**_guard;
    }

    void unlock() {
        _guard.unlock();
    }

    void relock() {
        _guard.relock();
    }

    void check() {
        GLOBED_DEBUG_ASSERT(_guard->has_value());
    }
private:
    asp::MutexGuard<std::optional<ConnectionInfo>, false> _guard;
};

class GLOBED_DLL NetworkManagerImpl {
public:
    NetworkManagerImpl();
    ~NetworkManagerImpl();

    static NetworkManagerImpl& get();

    void shutdown();

    Result<> connectCentral(std::string_view url);
    void disconnectCentral();

    qn::ConnectionState getConnState(bool game);

    void dumpNetworkStats();
    void simulateConnectionDrop();

    /// Returns the numeric ID of the preferred game server, or nullopt if not connected
    std::optional<uint8_t> getPreferredServer(bool useLatencyFallback = true);
    std::vector<GameServer> getGameServers();
    std::optional<GameServer> getGameServer(uint8_t id);
    /// Gets the game server the user is currently connected to, if any
    std::optional<GameServer> getGameServer();
    void setTemporaryServerOverride(std::optional<uint8_t> id);

    /// Returns whether the client is connected and authenticated with the central server
    bool isConnected() const;

    /// Returns whether the client is connected and authenticated with the game server
    bool isGameConnected() const;

    /// Returns the average latency to the game server, or 0 if not connected
    asp::time::Duration getGamePing();
    /// Returns the average latency to the central server, or 0 if not connected
    asp::time::Duration getCentralPing();

    /// Returns a unique identifier of the server (generated via hashing the server URL).
    /// Returns an empty string when not connected.
    std::string getCentralIdent();

    void clearAllUTokens();

    /// Returns the stored moderator password or an empty string if not connected or no password is stored
    std::string getStoredModPassword();
    void storeModPassword(const std::string& pw);

    /// Returns the tickrate to the connected game server, or 0 if not connected
    uint32_t getGameTickrate();
    std::vector<UserRole> getAllRoles();
    std::vector<UserRole> getUserRoles();
    std::vector<uint8_t> getUserRoleIds();
    std::optional<UserRole> getUserHighestRole();
    std::optional<UserRole> findRole(uint8_t roleId);
    std::optional<UserRole> findRole(std::string_view roleId);
    bool isModerator();
    bool isAuthorizedModerator();
    UserPermissions getUserPermissions();
    PunishReasons getModPunishReasons();
    std::optional<SpecialUserData> getOwnSpecialData();

    /// Returns the estimate packet loss to the game server over the last 5 seconds
    float getGameLoss();
    /// Returns the estimate packet loss to the game server over the last 1 minute
    float getGameLoss1Min();

    /// Force the client to resend user icons to the connected server. Does nothing if not connected.
    void invalidateIcons();
    /// Force the client to resend the friend list to the connected server. Does nothing if not connected.
    void invalidateFriendList();
    void markAuthorizedModerator();
    void deauthorizeMod();

    /// Get the ID of the current featured level on this server
    std::optional<FeaturedLevelMeta> getFeaturedLevel();
    bool hasViewedFeaturedLevel();
    void setViewedFeaturedLevel();

    void logQunetMessage(qn::log::Level level, const std::string& msg);
    void logArcMessage(arc::LogLevel level, const std::string& msg);

    // Message sending functions

    // Central server
    void sendUpdateUserSettings();
    void sendRoomStateCheck();
    void sendRequestRoomPlayers(const std::string& nameFilter);
    void sendRequestGlobalPlayerList(const std::string& nameFilter);
    void sendRequestLevelList();
    /// Requests player counts on these specific session ids
    void sendRequestPlayerCounts(std::span<const SessionId> sessions);
    /// Requests player counts on these levels, chooses the current room and server
    void sendRequestPlayerCounts(std::span<const int> levels);
    /// Requests player counts on this specific session id
    void sendRequestPlayerCounts(SessionId session);
    /// Requests player count on this level, chooses the current room and server
    void sendRequestPlayerCounts(int level);
    void sendCreateRoom(const std::string& name, uint32_t passcode, const RoomSettings& settings);
    void sendJoinRoom(uint32_t id, uint32_t passcode = 0);
    void sendJoinRoomByToken(uint64_t token);
    void sendLeaveRoom();
    void sendRequestRoomList(CStr nameFilter, uint32_t page);
    void sendAssignTeam(int accountId, uint16_t teamId);
    void sendCreateTeam(cocos2d::ccColor4B color);
    void sendDeleteTeam(uint16_t teamId);
    void sendUpdateTeam(uint16_t teamId, cocos2d::ccColor4B color);
    void sendGetTeamMembers();
    void sendRoomOwnerAction(RoomOwnerActionType type, int target = 0);
    void sendUpdateRoomSettings(const RoomSettings& settings);
    void sendInvitePlayer(int32_t player);
    void sendUpdatePinnedLevel(uint64_t sid);
    void sendFetchCredits();
    void sendGetDiscordLinkState();
    void sendSetDiscordPairingState(bool state);
    void sendDiscordLinkConfirm(int64_t id, bool confirm);
    void sendGetFeaturedList(uint32_t page);
    void sendGetFeaturedLevel();
    void sendSendFeaturedLevel(
        int32_t levelId,
        const std::string& levelName,
        int32_t authorId,
        const std::string& authorName,
        uint8_t rateTier,
        const std::string& note,
        bool queue
    );
    void sendNoticeReply(int32_t recipientId, const std::string& message);

    void sendAdminNotice(const std::string& message, const std::string& user, int roomId, int levelId, bool canReply, bool showSender);
    void sendAdminNoticeEveryone(const std::string& message);
    void sendAdminLogin(const std::string& password);
    void sendAdminKick(int32_t accountId, const std::string& message);
    void sendAdminFetchUser(const std::string& query);
    void sendAdminFetchUser(int32_t accountId);
    void sendAdminFetchMods();
    void sendAdminSetWhitelisted(int32_t accountId, bool whitelisted);
    void sendAdminCloseRoom(uint32_t roomId);
    void sendAdminFetchLogs(const FetchLogsFilters& filters);
    void sendAdminBan(int32_t accountId, const std::string& reason, int64_t expiresAt);
    void sendAdminUnban(int32_t accountId);
    void sendAdminRoomBan(int32_t accountId, const std::string& reason, int64_t expiresAt);
    void sendAdminRoomUnban(int32_t accountId);
    void sendAdminMute(int32_t accountId, const std::string& reason, int64_t expiresAt);
    void sendAdminUnmute(int32_t accountId);
    void sendAdminEditRoles(int32_t accountId, const std::vector<uint8_t>& roles);
    void sendAdminSetPassword(int32_t accountId, const std::string& password);
    void sendAdminUpdateUser(int32_t accountId, const std::string& username, int16_t cube, uint16_t color1, uint16_t color2, uint16_t glowColor);

    // Both servers
    void sendJoinSession(SessionId id, int author, bool platformer, bool editorCollab = false);
    void sendLeaveSession();

    // Game server
    void sendPlayerState(
        const PlayerState& state,
        const std::vector<int>& dataRequests,
        cocos2d::CCPoint cameraCenter,
        float cameraRadius
    );
    void queueLevelScript(const std::vector<EmbeddedScript>& scripts);
    void sendLevelScript(const std::vector<EmbeddedScript>& scripts);
    void queueGameEvent(OutEvent&& event);
    void sendVoiceData(const EncodedAudioFrame& frame);
    void sendQuickChat(uint32_t id);

    /// Listeners

    // threadSafe = true means event will be invoked earlier and on the arc thread
    template <typename T, typename F>
    [[nodiscard("listen returns a listener that must be kept alive to receive messages")]]
    MessageListener<T> listen(F&& callback, int priority = 0, bool threadSafe = false) {
        return MessageEvent<T>(threadSafe).listen(std::forward<F>(callback), priority);
    }

    template <typename T, typename F>
    geode::ListenerHandle* listenGlobal(F&& callback, int priority = 0, bool threadSafe = false) {
        return MessageEvent<T>(threadSafe).listen(std::forward<F>(callback), priority).leak();
    }

private:
    enum AuthKind {
        Utoken, Argon, Plain
    };

    asp::WeakPtr<arc::Runtime> m_runtime;
    std::shared_ptr<qn::Connection> m_centralConn, m_gameConn;
    arc::Notify m_workerNotify, m_gameWorkerNotify;
    WorkerState m_workerState;
    GameWorkerState m_gameWorkerState;
    std::optional<arc::mpsc::Sender<GameServerJoinRequest>> m_gameServerJoinTx;
    std::optional<ConnectionLogger> m_centralLogger;
    std::optional<ConnectionLogger> m_gameLogger;
    bool m_destructing = false;
    bool m_hasSecure = false;
    std::atomic<bool> m_debugLogs{false};
    std::atomic<bool> m_gameMustReauth{false};

    asp::Mutex<std::optional<ConnectionInfo>> m_connInfo;
    std::string m_connectingCentralUrl;
    asp::SpinLock<std::pair<std::string, bool>> m_abortCause;
    std::atomic<bool> m_manualDisconnect{false};

    arc::Future<> asyncInit();

    LockedConnInfo connInfo() const;

    Result<> onCentralDataReceived(CentralMessage::Reader& msg);
    Result<> onGameDataReceived(GameMessage::Reader& msg);

    void initializeTls();

    arc::Future<> threadWorkerLoop();
    arc::Future<> threadGameWorkerLoop();
    void threadPingGameServers(LockedConnInfo& info);
    void threadMaybeResendOwnData(LockedConnInfo& info);
    arc::Future<> threadTryAuth();
    arc::Future<> threadSetupLogger(bool central);
    void threadFlushLogger(bool central);

    void sendCentralAuth(AuthKind kind, const std::string& token = "");
    Result<> sendMessageToConnection(qn::Connection& conn, std::optional<ConnectionLogger>& logger, capnp::MallocMessageBuilder& msg, bool reliable, bool uncompressed);
    void sendToCentral(geode::FunctionRef<void(CentralMessage::Builder&)>&& func);
    void sendToGame(geode::FunctionRef<void(GameMessage::Builder&)>&& func, bool reliable = true, bool uncompressed = false);

    // Returns the user token for the current central server
    std::optional<std::string> getUToken();
    void setUToken(std::string token);
    void clearUToken();
    std::vector<uint8_t> computeUident(int accountId);

    // Returns the last known featured level ID on this server
    int32_t getLastFeaturedLevelId();
    void setLastFeaturedLevelId(int32_t id);

    void onCentralStateChanged(qn::ConnectionState state);
    void onGameStateChanged(qn::ConnectionState state);

    void abortConnection(std::string reason, bool silent = false);
    void handleSuccessfulLogin(LockedConnInfo& info);
    void handleLoginFailed(schema::main::LoginFailedReason reason);
    void showDisconnectCause(bool reconnecting, bool wasConnected);

    void joinSessionWith(LockedConnInfo& info, std::string_view serverUrl, uint8_t serverId, SessionId id, bool platformer, bool editorCollab);
    void sendGameLoginRequest(SessionId id = SessionId{}, bool platformer = false, bool editorCollab = false);
    void sendGameJoinRequest(SessionId id, bool platformer, bool editorCollab);

    template <typename Ft>
    void invokeListeners(Ft&& message) {
        using T = std::decay_t<Ft>;
        // invoke threadsafe listeners first
        auto res = MessageEvent<T>(true).send(message);
        if (res == geode::ListenerResult::Stop) {
            return;
        }

        // queue non-safe listeners to be invoked later
        FunctionQueue::get().queue([message = std::forward<Ft>(message)] mutable {
            MessageEvent<T>(false).send(message);
        });
    }
};

}