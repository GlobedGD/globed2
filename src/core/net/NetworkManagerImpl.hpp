#pragma once

#include <qunet/Connection.hpp>
#include <Geode/Result.hpp>
#include <globed/core/SessionId.hpp>
#include <globed/core/data/PlayerState.hpp>
#include <globed/core/data/RoomSettings.hpp>
#include <globed/core/data/Event.hpp>
#include <globed/core/net/MessageListener.hpp>
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
    uint32_t lastLatency = -1;
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

    // Message sending functions

    // Central server
    void sendRoomStateCheck();
    void sendCreateRoom(const std::string& name, uint32_t passcode, const RoomSettings& settings);
    void sendJoinRoom(uint32_t id, uint32_t passcode = 0);
    void sendLeaveRoom();
    void sendRequestRoomList();

    // Both servers
    void sendJoinSession(SessionId id);
    void sendLeaveSession();

    // Game server
    void sendPlayerState(const PlayerState& state, const std::vector<int>& dataRequests);
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
    std::atomic<qn::ConnectionState> m_centralConnState;
    std::string m_centralUrl;
    std::string m_knownArgonUrl;
    bool m_waitingForArgon = false;
    bool m_established = false;

    qn::Connection m_gameConn;
    std::atomic<qn::ConnectionState> m_gameConnState;
    std::string m_gameServerUrl;
    std::optional<std::pair<std::string, SessionId>> m_gsDeferredConnectJoin;
    std::optional<SessionId> m_gsDeferredJoin;
    std::queue<Event> m_gameEventQueue;
    bool m_gameEstablished = false;

    std::unordered_map<std::string, GameServer> m_gameServers;

    // Note: this mutex is recursive so that listeners can be added/removed inside listener callbacks
    asp::Mutex<std::unordered_map<std::type_index, std::vector<void*>>, true> m_listeners;

    void onCentralConnected();
    void onCentralDisconnected();
    geode::Result<> onCentralDataReceived(CentralMessage::Reader& msg);
    geode::Result<> onGameDataReceived(GameMessage::Reader& msg);
    geode::Result<> sendToCentral(std::function<void(CentralMessage::Builder&)> func);
    geode::Result<> sendToGame(std::function<void(GameMessage::Builder&)> func, bool reliable = true);

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
