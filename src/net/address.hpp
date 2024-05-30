#pragma once
#include <string_view>

// for sockaddr_in
#ifdef GEODE_IS_WINDOWS
# include <WinSock2.h>
#else
# include <netinet/in.h>
#endif

// Represents an IPv4 address and a port
class NetworkAddress {
    static constexpr uint16_t DEFAULT_PORT = 41001;
    static inline std::unordered_map<std::string, in_addr> gaiCache;

public:
    NetworkAddress();

    // Parses the given string into an IP address and an optional port
    NetworkAddress(const std::string_view address);

    // Constructs a `NetworkAddress` from the given ip address and port
    NetworkAddress(const std::string_view ip, uint16_t port);

    NetworkAddress(const NetworkAddress&) = default;
    NetworkAddress& operator=(const NetworkAddress&) = default;

    void set(const std::string_view address);
    void set(const std::string_view ip, uint16_t port);

    // Returns the input in format `ip:port`. IP address is not resolved if a domain name was provided.
    std::string toString() const;

    // Returns a `sockaddr_in` struct corresponding to this `NetworkAddress`.
    // Note that this might block for DNS lookup if earlier provided address was not an IP address.
    geode::Result<sockaddr_in> resolve() const;

    // Same as `resolve` except returns as a string.
    geode::Result<std::string> resolveToString() const;

private:
    std::string ip;
    uint16_t port;
};