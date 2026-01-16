#pragma once

#include "_internal.hpp"

namespace globed::net {
// Returns whether the player is currently connected to a server.
Result<bool> isConnected();

// Returns the TPS (ticks per second) of the server, or 0 if not connected.
// This value indicates how many times player data is sent between client and server every second, while in a level.
Result<uint32_t> getServerTps();

// Returns the current latency to the server, or -1 if not connected.
Result<uint32_t> getPing();

// Returns whether the user is connected to a standalone server.
Result<bool> isStandalone();

// Returns whether a connection break happened and the client is currently trying to reconnect.
Result<bool> isReconnecting();
} // namespace globed::net

// Implementation

namespace globed::net {
inline Result<bool> isConnected()
{
    return _internal::request<bool>(_internal::Type::IsConnected);
}

inline Result<uint32_t> getServerTps()
{
    return _internal::request<uint32_t>(_internal::Type::ServerTps);
}

inline Result<uint32_t> getPing()
{
    return _internal::request<uint32_t>(_internal::Type::Ping);
}

inline Result<bool> isStandalone()
{
    return _internal::request<bool>(_internal::Type::IsStandalone);
}

inline Result<bool> isReconnecting()
{
    return _internal::request<bool>(_internal::Type::IsReconnecting);
}
} // namespace globed::net
