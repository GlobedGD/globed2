#pragma once

#include <qunet/Connection.hpp>
#include <Geode/Result.hpp>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include "data/generated.hpp"

namespace globed {

using CentralMessage = globed::schema::main::Message;

class NetworkManagerImpl {
public:
    NetworkManagerImpl();
    ~NetworkManagerImpl();

    geode::Result<> connectCentral(std::string_view url);
    geode::Result<> disconnectCentral();

private:
    qn::Connection m_centralConn;
    std::string m_centralUrl;
    std::string m_knownArgonUrl;
    bool m_waitingForArgon = false;

    void onCentralConnected();
    void onCentralDisconnected();
    geode::Result<> onCentralDataReceived(CentralMessage::Reader& msg);
    geode::Result<> sendToCentral(std::function<void(CentralMessage::Builder&)> func);

    // Returns the user token for the current central server
    std::optional<std::string> getUToken();
    void setUToken(std::string token);
    void clearUToken();

    void tryAuth();
    void doArgonAuth(std::string token);
    void abortConnection(std::string reason);

    // Handlers for messages
    void handleLoginFailed(schema::main::LoginFailedReason reason);
};

}
