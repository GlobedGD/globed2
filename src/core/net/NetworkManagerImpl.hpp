#pragma once

#include <qunet/Connection.hpp>
#include <Geode/Result.hpp>
#include <globed/core/SessionId.hpp>
#include <globed/core/game/PlayerState.hpp>

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
    void sendPlayerState(const PlayerState& state);

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
};

}
