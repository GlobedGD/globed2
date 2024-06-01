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
    static inline std::unordered_map<std::string, in_addr> dnsCache;

public:
    static constexpr uint16_t DEFAULT_PORT = 4202;

    NetworkAddress();

    // Parses the given string in format `host:port`
    NetworkAddress(const std::string_view address);

    // Constructs a `NetworkAddress` from the given host and port
    NetworkAddress(const std::string_view host, uint16_t port);

    NetworkAddress(const NetworkAddress&) = default;
    NetworkAddress& operator=(const NetworkAddress&) = default;

    void set(const std::string_view address);
    void set(const std::string_view host, uint16_t port);

    // Returns the input in format `host:port`. If the host is a domain name, it is not resolved to an IP address.
    std::string toString() const;

    // Returns a `sockaddr_in` struct corresponding to this `NetworkAddress`.
    // Note that this might block for DNS lookup if contained host was not an IP address.
    geode::Result<sockaddr_in> resolve() const;

    // Combination of `resolve` and `toString`, returns the input in format `host:port` but does do DNS resolution.
    // Note that this might block for DNS lookup if contained host was not an IP address.
    geode::Result<std::string> resolveToString() const;

private:
    std::string host;
    uint16_t port;
};