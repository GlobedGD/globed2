#include "net.hpp"

#include <defs/geode.hpp>
#include <util/format.hpp>

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
        return fmt::format("globed-geode-xd/{}; {}", Mod::get()->getVersion().toString(), GLOBED_PLATFORM_STRING);
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
        struct addrinfo hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        struct addrinfo* result;

        if (0 != ::getaddrinfo(std::string(hostname).c_str(), nullptr, &hints, &result)) {
            return Err(util::net::lastErrorString());
        }

        GLOBED_REQUIRE_SAFE(result->ai_family == AF_INET, "getaddrinfo returned invalid address family")
        GLOBED_REQUIRE_SAFE(result->ai_socktype == SOCK_DGRAM, "getaddrinfo returned invalid socket type")
        GLOBED_REQUIRE_SAFE(result->ai_protocol == IPPROTO_UDP, "getaddrinfo returned invalid ip protocol")
        auto* ipaddr = (struct sockaddr_in*) result->ai_addr;

        std::string out;
        out.resize(16);

        auto ntopResult = inet_ntop(AF_INET, &ipaddr->sin_addr, out.data(), 16);
        ::freeaddrinfo(result);

        if (ntopResult == nullptr) {
            return Err(util::net::lastErrorString());
        }

        out.resize(std::strlen(out.c_str()));

        return Ok(out);
    }

    Result<> initSockaddr(const std::string_view address, unsigned short port, sockaddr_in& dest) {
        dest.sin_port = htons(port);

        bool validIp = (inet_pton(AF_INET, std::string(address).c_str(), &dest.sin_addr) > 0);
        if (validIp) {
            return Ok();
        } else {
            // if not a valid IPv4, assume it's a domain and try getaddrinfo
            auto result = util::net::getaddrinfo(address);
            GLOBED_UNWRAP_INTO(result, auto ip);

            validIp = (inet_pton(AF_INET, ip.c_str(), &dest.sin_addr) > 0);
            GLOBED_REQUIRE_SAFE(validIp, "invalid address was returned by getaddrinfo")

            return Ok();
        }
    }
}

