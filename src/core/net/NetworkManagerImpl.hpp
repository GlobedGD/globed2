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
#include <globed/core/data/ModPermissions.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/util/FunctionQueue.hpp>
#include <modules/scripting/data/EmbeddedScript.hpp>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <std23/function_ref.h>
#include "data/generated.hpp"
#include <qunet/Connection.hpp>
#include <Geode/Result.hpp>
#include <typeindex>

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

    void updateLatency(uint32_t latency);
};

struct DeferredSessionJoin {
    SessionId id;
    bool platformer;
    bool editorCollab;
};


// Struct for fields that are the same across one connection but are cleared on disconnect
struct ConnectionInfo {
    std::string m_knownArgonUrl;
    bool m_waitingForArgon = false;
    bool m_established = false;
    bool m_authenticating = false;
    asp::time::Instant m_triedAuthAt;

    std::unordered_map<std::string, GameServer> m_gameServers;

    std::atomic<qn::ConnectionState> m_gameConnState;
    std::string m_gameServerUrl;
    std::optional<std::pair<std::string, DeferredSessionJoin>> m_gsDeferredConnectJoin;
    std::optional<DeferredSessionJoin> m_gsDeferredJoin;
    std::queue<OutEvent> m_gameEventQueue;
    std::vector<EmbeddedScript> m_queuedScripts;
    bool m_gameEstablished = false;

    uint32_t m_gameTickrate = 0;
    std::optional<FeaturedLevelMeta> m_featuredLevel;
    std::vector<UserRole> m_allRoles;
    std::vector<UserRole> m_userRoles;
    std::vector<uint8_t> m_userRoleIds;
    std::optional<MultiColor> m_nameColor;
    ModPermissions m_perms{};
    bool m_canNameRooms = false;
    PunishReasons m_punishReasons{};
    bool m_authorizedModerator;

    bool m_sentIcons = true; // icons are sent at login
    bool m_sentFriendList = false;

    void startedAuth() {
        m_authenticating = true;
        m_triedAuthAt = asp::time::Instant::now();
    }
};

class NetworkManagerImpl {
public:
    NetworkManagerImpl();
    ~NetworkManagerImpl();

    static NetworkManagerImpl& get();

    geode::Result<> connectCentral(std::string_view url);
    geode::Result<> disconnectCentral();
    geode::Result<> cancelConnection();

    qn::ConnectionState getConnState(bool game);

    void dumpNetworkStats();

    /// Returns the numeric ID of the preferred game server, or nullopt if not connected
    std::optional<uint8_t> getPreferredServer();
    std::vector<GameServer> getGameServers();

    /// Returns whether the client is connected and authenticated with the central server
    bool isConnected() const;

    /// Returns the average latency to the game server, or 0 if not connected
    asp::time::Duration getGamePing();
    /// Returns the average latency to the central server, or 0 if not connected
    asp::time::Duration getCentralPing();

    /// Returns a unique identifier of the server (generated via hashing the server URL).
    /// Returns an empty string when not connected.
    std::string getCentralIdent();

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
    ModPermissions getModPermissions();
    PunishReasons getModPunishReasons();
    std::optional<SpecialUserData> getOwnSpecialData();

    bool canNameRooms();

    /// Force the client to resend user icons to the connected server. Does nothing if not connected.
    void invalidateIcons();
    /// Force the client to resend the friend list to the connected server. Does nothing if not connected.
    void invalidateFriendList();
    void markAuthorizedModerator();

    /// Get the ID of the current featured level on this server
    std::optional<FeaturedLevelMeta> getFeaturedLevel();
    bool hasViewedFeaturedLevel();
    void setViewedFeaturedLevel();

    // Message sending functions

    // Central server
    void sendUpdateUserSettings();
    void sendRoomStateCheck();
    void sendRequestRoomPlayers(const std::string& nameFilter);
    void sendRequestGlobalPlayerList(const std::string& nameFilter);
    void sendRequestLevelList();
    void sendRequestPlayerCounts(const std::vector<uint64_t>& sessions);
    void sendRequestPlayerCounts(std::span<const uint64_t> sessions);
    void sendRequestPlayerCounts(std::span<const SessionId> sessions);
    void sendRequestPlayerCounts(uint64_t session);
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

    void sendAdminLogin(const std::string& password);
    void sendAdminKick(int32_t accountId, const std::string& message);
    void sendAdminNotice(const std::string& message, const std::string& user, int roomId, int levelId, bool canReply, bool showSender);
    void sendAdminNoticeEveryone(const std::string& message);
    void sendAdminFetchUser(const std::string& query);
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
    void sendAdminFetchMods();
    void sendAdminSetWhitelisted(int32_t accountId, bool whitelisted);
    void sendAdminCloseRoom(uint32_t roomId);

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

    // Listeners

    template <typename T>
    [[nodiscard("listen returns a listener that must be kept alive to receive messages")]]
    MessageListener<T> listen(ListenerFn<T> callback) {
        auto listener = new MessageListenerImpl<T>(std::move(callback));
        this->addListener(typeid(T), listener, (void*) +[](void* ptr) {
            delete static_cast<MessageListenerImpl<T>*>(ptr);
        });
        return MessageListener<T>(listener);
    }

    template <typename T>
    MessageListenerImpl<T>* listenGlobal(ListenerFn<T> callback) {
        auto listener = new MessageListenerImpl<T>(std::move(callback));
        this->addListener(typeid(T), listener, (void*) +[](void* ptr) {
            delete static_cast<MessageListenerImpl<T>*>(ptr);
        });
        return listener;
    }

    void addListener(const std::type_info& ty, void* listener, void* dtor);
    void removeListener(const std::type_info& ty, void* listener);

private:
    enum AuthKind {
        Utoken, Argon, Plain
    };

    qn::Connection m_centralConn;
    qn::Connection m_gameConn;

    std::string m_centralUrl;
    std::atomic<qn::ConnectionState> m_centralConnState;
    asp::Mutex<std::optional<ConnectionInfo>, true> m_connInfo;

    asp::Mutex<std::string> m_pendingConnectUrl;
    asp::Notify m_pendingConnectNotify;
    asp::Notify m_disconnectNotify;
    asp::AtomicBool m_disconnectRequested;
    asp::AtomicBool m_manualDisconnect = false;
    asp::AtomicBool m_tryingReconnect = false;
    asp::Mutex<std::pair<std::string, bool>> m_abortCause;
    asp::Notify m_finishedClosingNotify;
    bool m_destructing = false;
    bool m_hasSecure = false;

    // Note: this mutex is recursive so that listeners can be added/removed inside listener callbacks
    asp::Mutex<std::unordered_map<std::type_index, std::vector<std::pair<void*, void*>>>, true> m_listeners;
    asp::Thread<> m_workerThread;

    void onCentralConnected();
    void onCentralDisconnected();
    geode::Result<> onCentralDataReceived(CentralMessage::Reader& msg);
    geode::Result<> onGameDataReceived(GameMessage::Reader& msg);

    static Result<> sendMessageToConnection(qn::Connection& conn, capnp::MallocMessageBuilder& msg, bool reliable, bool uncompressed);
    void sendToCentral(std23::function_ref<void(CentralMessage::Builder&)>&& func);
    void sendToGame(std23::function_ref<void(GameMessage::Builder&)>&& func, bool reliable = true, bool uncompressed = false);

    void maybeTryReconnect();
    void disconnectInner();
    void resetGameVars();

    // Returns the user token for the current central server
    std::optional<std::string> getUToken();
    void setUToken(std::string token);
    void clearUToken();

    // Returns the last known featured level ID on this server
    int32_t getLastFeaturedLevelId();
    void setLastFeaturedLevelId(int32_t id);

    std::vector<uint8_t> computeUident(int accountId);

    void tryAuth();
    void sendCentralAuth(AuthKind kind, const std::string& token = "");
    void abortConnection(std::string reason, bool silent = false);

    void joinSessionWith(std::string_view serverUrl, SessionId id, bool platformer, bool editorCollab = false);
    void sendGameLoginRequest(SessionId id = SessionId{}, bool platformer = false, bool editorCollab = false);
    void sendGameJoinRequest(SessionId id, bool platformer, bool editorCollab);

    // Handlers for messages
    void handleLoginFailed(schema::main::LoginFailedReason reason);

    // Thread functions
    void thrPingGameServers(ConnectionInfo& info);
    void thrMaybeResendOwnData(ConnectionInfo& info);

    template <typename T>
    void invokeListeners(T&& message) {
        auto listenersLock = m_listeners.lock();
        auto listeners = listenersLock->find(typeid(std::decay_t<T>));

        // check if there are any non-threadsafe listeners, and defer to the main thread if so
        bool hasThreadUnsafe = false;

        if (listeners != listenersLock->end()) {
            for (auto& [listener, _] : listeners->second) {
                auto impl = static_cast<MessageListenerImpl<T>*>(listener);

                if (!impl->isThreadSafe()) {
                    hasThreadUnsafe = true;
                    break;
                }
            }
        }

        if (hasThreadUnsafe) {
            FunctionQueue::get().queue([this, message = std::forward<T>(message)]() mutable {
                this->invokeUnchecked(std::forward<T>(message));
            });
        } else {
            listenersLock.unlock(); // prevent deadlocks
            this->invokeUnchecked(std::forward<T>(message));
        }
    }

    /// Like `invokeListeners`, but does not check for thread safety and invokes the listeners directly.
    template <typename T>
    void invokeUnchecked(T&& message) {
        auto listenersLock = m_listeners.lock();
        auto listeners = listenersLock->find(typeid(T));

        if (listeners == listenersLock->end()) {
            geode::log::debug("No listeners for message type '{}'", typeid(T).name());
            return;
        }

        for (auto& [listener, _] : listeners->second) {
            auto impl = static_cast<MessageListenerImpl<T>*>(listener);

            if (impl->invoke(message) == ListenerResult::Stop) {
                break;
            }
        }
    }
};

}
