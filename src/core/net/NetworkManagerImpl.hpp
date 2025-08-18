#pragma once

#include <qunet/Connection.hpp>
#include <Geode/Result.hpp>
#include <globed/core/SessionId.hpp>
#include <globed/core/data/PlayerState.hpp>
#include <globed/core/data/RoomSettings.hpp>
#include <globed/core/data/UserRole.hpp>
#include <globed/core/data/Event.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <modules/scripting/data/EmbeddedScript.hpp>
#include <typeindex>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include "data/generated.hpp"

namespace globed {

using CentralMessage = globed::schema::main::Message;
using GameMessage = globed::schema::game::Message;

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

// Struct for fields that are the same across one connection but are cleared on disconnect
struct ConnectionInfo {
    std::string m_knownArgonUrl;
    bool m_waitingForArgon = false;
    bool m_established;

    std::unordered_map<std::string, GameServer> m_gameServers;

    std::atomic<qn::ConnectionState> m_gameConnState;
    std::string m_gameServerUrl;
    std::optional<std::pair<std::string, SessionId>> m_gsDeferredConnectJoin;
    std::optional<SessionId> m_gsDeferredJoin;
    std::queue<Event> m_gameEventQueue;
    bool m_gameEstablished = false;

    uint32_t m_gameTickrate = 0;
    std::vector<UserRole> m_allRoles;
    std::vector<UserRole> m_userRoles;
    bool m_isModerator = false;
    bool m_isAuthorizedModerator = false;

    bool m_sentIcons = true; // icons are sent at login
    bool m_sentFriendList = false;
};

class NetworkManagerImpl {
public:
    NetworkManagerImpl();
    ~NetworkManagerImpl();

    static NetworkManagerImpl& get();

    geode::Result<> connectCentral(std::string_view url);
    geode::Result<> disconnectCentral();

    qn::ConnectionState getConnState(bool game);

    /// Returns the numeric ID of the preferred game server, or nullopt if not connected
    std::optional<uint8_t> getPreferredServer();

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
    std::optional<UserRole> getUserHighestRole();
    std::optional<UserRole> findRole(uint8_t roleId);
    std::optional<UserRole> findRole(std::string_view roleId);
    bool isModerator();
    bool isAuthorizedModerator();

    /// Force the client to resend user icons to the connected server. Does nothing if not connected.
    void invalidateIcons();
    /// Force the client to resend the friend list to the connected server. Does nothing if not connected.
    void invalidateFriendList();
    void markAuthorizedModerator();

    // Message sending functions

    // Central server
    void sendRoomStateCheck();
    void sendCreateRoom(const std::string& name, uint32_t passcode, const RoomSettings& settings);
    void sendJoinRoom(uint32_t id, uint32_t passcode = 0);
    void sendLeaveRoom();
    void sendRequestRoomList();
    void sendAssignTeam(int accountId, uint16_t teamId);
    void sendCreateTeam(cocos2d::ccColor4B color);
    void sendDeleteTeam(uint16_t teamId);
    void sendUpdateTeam(uint16_t teamId, cocos2d::ccColor4B color);
    void sendGetTeamMembers();
    void sendAdminNotice(const std::string& message, const std::string& user, int roomId, int levelId, bool canReply);
    void sendAdminNoticeEveryone(const std::string& message);
    void sendAdminLogin(const std::string& password);
    void sendAdminFetchUser(const std::string& query);
    void sendAdminFetchMods();

    // Both servers
    void sendJoinSession(SessionId id);
    void sendLeaveSession();

    // Game server
    void sendPlayerState(const PlayerState& state, const std::vector<int>& dataRequests);
    void sendLevelScript(const std::vector<EmbeddedScript>& scripts);
    void queueGameEvent(Event&& event);

    // Listeners

    template <typename T>
    [[nodiscard("listen returns a listener that must be kept alive to receive messages")]]
    MessageListener<T> listen(ListenerFn<T> callback) {
        auto listener = new MessageListenerImpl<T>(std::move(callback));
        this->addListener(typeid(T), listener);
        return MessageListener<T>(listener);
    }

    template <typename T>
    MessageListenerImpl<T>* listenGlobal(ListenerFn<T> callback) {
        auto listener = new MessageListenerImpl<T>(std::move(callback));
        this->addListener(typeid(T), listener);
        return listener;
    }

    void addListener(const std::type_info& ty, void* listener);
    void removeListener(const std::type_info& ty, void* listener);

private:
    qn::Connection m_centralConn;
    qn::Connection m_gameConn;

    std::string m_centralUrl;
    std::atomic<qn::ConnectionState> m_centralConnState;
    asp::Mutex<std::optional<ConnectionInfo>, true> m_connInfo;

    asp::Mutex<std::string> m_pendingConnectUrl;
    asp::Notify m_pendingConnectNotify;
    asp::Notify m_disconnectNotify;
    asp::AtomicBool m_disconnectRequested;
    asp::Notify m_finishedClosingNotify;

    // Note: this mutex is recursive so that listeners can be added/removed inside listener callbacks
    asp::Mutex<std::unordered_map<std::type_index, std::vector<void*>>, true> m_listeners;
    asp::Thread<> m_workerThread;

    void onCentralConnected();
    void onCentralDisconnected();
    geode::Result<> onCentralDataReceived(CentralMessage::Reader& msg);
    geode::Result<> onGameDataReceived(GameMessage::Reader& msg);
    geode::Result<> sendToCentral(std::function<void(CentralMessage::Builder&)> func);
    geode::Result<> sendToGame(std::function<void(GameMessage::Builder&)> func, bool reliable = true);

    void resetGameVars();

    // Returns the user token for the current central server
    std::optional<std::string> getUToken();
    void setUToken(std::string token);
    void clearUToken();

    void tryAuth();
    void doArgonAuth(std::string token);
    void abortConnection(std::string reason);

    void joinSessionWith(std::string_view serverUrl, SessionId id);
    void sendGameLoginJoinRequest(SessionId id);
    void sendGameLoginRequest();
    void sendGameJoinRequest(SessionId id);

    // Handlers for messages
    void handleLoginFailed(schema::main::LoginFailedReason reason);

    // Thread functions
    void thrPingGameServers();
    void thrMaybeResendOwnData();

    template <typename T>
    void invokeListeners(T&& message) {
        auto listenersLock = m_listeners.lock();
        auto listeners = listenersLock->find(typeid(std::decay_t<T>));

        // check if there are any non-threadsafe listeners, and defer to the main thread if so
        bool hasThreadUnsafe = false;

        if (listeners != listenersLock->end()) {
            for (auto* listener : listeners->second) {
                auto impl = static_cast<MessageListenerImpl<T>*>(listener);

                if (!impl->isThreadSafe()) {
                    hasThreadUnsafe = true;
                    break;
                }
            }
        }

        if (hasThreadUnsafe) {
            geode::Loader::get()->queueInMainThread([this, message = std::forward<T>(message)]() mutable {
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

        for (auto* listener : listeners->second) {
            auto impl = static_cast<MessageListenerImpl<T>*>(listener);

            if (impl->invoke(message) == ListenerResult::Stop) {
                break;
            }
        }
    }
};

}
