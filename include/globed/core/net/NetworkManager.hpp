#pragma once

#include <globed/util/singleton.hpp>
#include <memory>

namespace globed {

class NetworkManagerImpl;

enum class ConnectionState {
    Disconnected,
    DnsResolving,
    Pinging,
    Connecting,
    Connected,
    Closing,
    Reconnecting,
};

class NetworkManager : public SingletonBase<NetworkManager> {
public:
    // Connect to the central server at the given URL.
    // See qunet's documentation for the URL format.
    geode::Result<> connectCentral(std::string_view url);

    ConnectionState getConnectionState();

private:
    friend class SingletonBase;
    friend class NetworkManagerImpl;

    std::unique_ptr<NetworkManagerImpl> m_impl;

    NetworkManager();
    ~NetworkManager();
};

}
