#include <globed/core/net/NetworkManager.hpp>

#include "NetworkManagerImpl.hpp"

using namespace geode::prelude;

namespace globed {

NetworkManager::NetworkManager() : m_impl(std::make_unique<NetworkManagerImpl>()) {}

NetworkManager::~NetworkManager() {
    log::info("goodbye from networkmanager!");
}

Result<> NetworkManager::connectCentral(std::string_view url) {
    return m_impl->connectCentral(url);
}

ConnectionState NetworkManager::getConnectionState() {
    switch (m_impl->getConnState(false)) {
        case qn::ConnectionState::Disconnected:
            return ConnectionState::Disconnected;
        case qn::ConnectionState::DnsResolving:
            return ConnectionState::DnsResolving;
        case qn::ConnectionState::Pinging:
            return ConnectionState::Pinging;
        case qn::ConnectionState::Connecting:
            return ConnectionState::Connecting;
        case qn::ConnectionState::Connected:
            return m_impl->isConnected() ? ConnectionState::Connected : ConnectionState::Authenticating;
        case qn::ConnectionState::Closing:
            return ConnectionState::Closing;
        case qn::ConnectionState::Reconnecting:
            return ConnectionState::Reconnecting;
    }
}

}