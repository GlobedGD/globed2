#pragma once

namespace globed {

enum class ConnectionState {
    Disconnected,
    DnsResolving,
    Pinging,
    Connecting,
    Authenticating,
    Connected,
    Closing,
    Reconnecting,
};

}
