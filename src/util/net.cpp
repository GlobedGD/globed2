#include "net.hpp"

#include <defs/geode.hpp>
#include <util/format.hpp>

#ifdef GEODE_IS_WINDOWS
# include <ws2tcpip.h>
#else
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
#endif

using namespace geode::prelude;

namespace util::net {
    std::string lastErrorString(bool gai) {
        return lastErrorString(lastErrorCode(), gai);
    }

    [[noreturn]] void throwLastError(bool gai) {
        auto message = lastErrorString(gai);
        log::error("Throwing network exception: {}", message);
        throw std::runtime_error(std::string("Network error: ") + message);
    }

    std::string webUserAgent() {
        return fmt::format(
            "globed-geode-xd/{}; Globed {}; Loader {}",
            Mod::get()->getVersion().toString(),
            GLOBED_PLATFORM_STRING,
            Loader::get()->getVersion().toString()
        );
    }

    std::string loginPlatformString() {
#ifdef GLOBED_DEBUG
        return fmt::format(
            "{} ({}, Geode {})",
            GLOBED_PLATFORM_STRING,
            Mod::get()->getVersion().toString(),
            Loader::get()->getVersion().toString()
        );
#else
        // no telemetry in release :(
        return fmt::format("Globed {}", Mod::get()->getVersion().toString());
#endif
    }

    Result<std::pair<std::string, unsigned short>> splitAddress(const std::string_view address, unsigned short defaultPort) {
        std::pair<std::string, unsigned short> out;

        size_t colon = address.find(':');
        bool hasPort = colon != std::string::npos && colon != 0;

        GLOBED_REQUIRE_SAFE(hasPort || defaultPort != 0, "invalid address, cannot split into IP and port")

        if (hasPort) {
            out.first = address.substr(0, colon);
            auto portResult = util::format::parse<unsigned short>(address.substr(colon + 1));

            if (!portResult.has_value() && defaultPort == 0) {
                return Err("invalid port number");
            }

            out.second = portResult.value_or(defaultPort);
        } else {
            out.first = address;
            out.second = defaultPort;
        }

        return Ok(out);
    }

    bool sameSockaddr(const sockaddr_in& s1, const sockaddr_in& s2) {
        if (s1.sin_family != s2.sin_family || s1.sin_port != s2.sin_port) {
            return false;
        }

        return std::memcmp(&s1.sin_addr, &s2.sin_addr, sizeof(s1.sin_addr)) == 0;
    }

    Result<std::string> getaddrinfo(const std::string_view hostname) {
        auto ipaddr = std::make_unique<sockaddr_in>();

        GLOBED_UNWRAP(getaddrinfo(hostname, *ipaddr));

        return inAddrToString(ipaddr->sin_addr);
    }

    Result<> getaddrinfo(const std::string_view hostname, sockaddr_in& out) {
        struct addrinfo hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        struct addrinfo* result;

        if (0 != ::getaddrinfo(std::string(hostname).c_str(), nullptr, &hints, &result)) {
            return Err(util::net::lastErrorString());
        }

        if (result->ai_family != AF_INET || result->ai_socktype != SOCK_DGRAM || result->ai_protocol != IPPROTO_UDP) {
            ::freeaddrinfo(result);
            return Err("getaddrinfo returned wrong output");
        }

        auto* ipaddr = (struct sockaddr_in*) result->ai_addr;

        std::memcpy(&out, ipaddr, sizeof(sockaddr_in));

        ::freeaddrinfo(result);

        return Ok();
    }

    Result<std::string> inAddrToString(const in_addr& addr) {
        std::string out;
        out.resize(16);

        auto ntopResult = inet_ntop(AF_INET, &addr, out.data(), 16);

        if (ntopResult == nullptr) {
            return Err(lastErrorString());
        }

        out.resize(std::strlen(out.c_str()));

        return Ok(std::move(out));
    }

    Result<> stringToInAddr(const char* addr, in_addr& out) {
        auto ptonResult = inet_pton(AF_INET, addr, &out);

        if (ptonResult > 0) {
            return Ok();
        } else {
            return Err(lastErrorString());
        }
    }

    uint16_t hostToNetworkPort(uint16_t port) {
        return ::htons(port);
    }
}

