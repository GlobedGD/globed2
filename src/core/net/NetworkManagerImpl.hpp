#pragma once

#include <qunet/Connection.hpp>
#include <Geode/Result.hpp>
#include <globed/core/SessionId.hpp>
#include <globed/core/data/PlayerState.hpp>
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

    /// Returns the numeric ID of the preferred game server, or nullopt if not connected
    std::optional<uint8_t> getPreferredServer();

    /// Returns whether the client is connected and authenticated with the central server
    bool isConnected() const;

    void joinSession(SessionId id);
    void leaveSession();
    void sendPlayerState(const PlayerState& state, const std::vector<int>& dataRequests);

    template <typename T>
    MessageListener<T> listen(ListenerFn<T> callback) {
        auto listener = new MessageListenerImpl<T>(std::move(callback));
        this->addListener(typeid(T), listener);
        return MessageListener<T>(listener);
    }

    void addListener(const std::type_info& ty, void* listener);
    void removeListener(const std::type_info& ty, void* listener);

private:
    qn::Connection m_centralConn;
    std::string m_centralUrl;
    std::string m_knownArgonUrl;
    bool m_waitingForArgon = false;
    bool m_established = false;

    qn::Connection m_gameConn;
    std::string m_gameServerUrl;
    std::optional<std::pair<std::string, SessionId>> m_gsDeferredConnectJoin;
    std::optional<SessionId> m_gsDeferredJoin;
    bool m_gameEstablished = false;

    std::unordered_map<std::string, GameServer> m_gameServers;

    asp::Mutex<std::unordered_map<std::type_index, std::vector<void*>>> m_listeners;

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
        auto listeners = listenersLock->find(typeid(T));

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
