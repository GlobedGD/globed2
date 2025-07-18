#include "net.hpp"

#include <defs/geode.hpp>
#include <managers/settings.hpp>
#include <managers/web.hpp>
#include <util/format.hpp>
#include <util/data.hpp>
#include <util/debug.hpp>

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

    static const char* realPlatform() {
#ifdef GEODE_IS_WINDOWS
# define GLOBED_WINE_PLATFORM_STRING GLOBED_PLATFORM_STRING " (Wine)"
        return util::debug::isWine() ? GLOBED_WINE_PLATFORM_STRING : GLOBED_PLATFORM_STRING;
#else
        return GLOBED_PLATFORM_STRING;
#endif
    }

    std::string webUserAgent() {
        return fmt::format(
            "globed-geode-xd/{}; Globed {}; Loader {}",
            Mod::get()->getVersion().toVString(),
            realPlatform(),
            Loader::get()->getVersion().toVString()
        );
    }

    std::string loginPlatformString() {
        return fmt::format(
            "{} ({}, Geode {})",
            realPlatform(),
            Mod::get()->getVersion().toVString(),
            Loader::get()->getVersion().toVString()
        );
    }

    Result<std::pair<std::string, unsigned short>> splitAddress(std::string_view address, unsigned short defaultPort) {
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

    bool sameSockaddr(const sockaddr_storage& s1, const sockaddr_storage& s2) {
        if (s1.ss_family != s2.ss_family) {
            return false;
        }

        if (s1.ss_family == AF_INET) {
            return sameSockaddr(reinterpret_cast<const sockaddr_in&>(s1).sin_addr, reinterpret_cast<const sockaddr_in&>(s2).sin_addr);
        } else if (s1.ss_family == AF_INET6) {
            return sameSockaddr(reinterpret_cast<const sockaddr_in6&>(s1).sin6_addr, reinterpret_cast<const sockaddr_in6&>(s2).sin6_addr);
        } else {
            // unsupported address family
            return false;
        }
    }

    bool sameSockaddr(const in_addr& s1, const in_addr& s2) {
        return std::memcmp(&s1, &s2, sizeof(in_addr)) == 0;
    }

    bool sameSockaddr(const in6_addr& s1, const in6_addr& s2) {
        return std::memcmp(&s1, &s2, sizeof(in6_addr)) == 0;
    }

    Result<std::string> getaddrinfo(std::string_view hostname) {
        auto ipaddr = std::make_unique<sockaddr_storage>();

        GLOBED_UNWRAP(getaddrinfo(hostname, *ipaddr));

        if (ipaddr->ss_family == AF_INET) {
            return inAddrToString(((sockaddr_in*)(ipaddr.get()))->sin_addr);
        } else {
            return inAddrToString(((sockaddr_in6*)(ipaddr.get()))->sin6_addr);
        }
    }

    Result<> getaddrinfo(std::string_view hostname, sockaddr_storage& out) {
        struct addrinfo hints = {};
        hints.ai_family = util::net::activeAddressFamily();
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        struct addrinfo* result;

        if (0 != ::getaddrinfo(std::string(hostname).c_str(), nullptr, &hints, &result)) {
            auto code = util::net::lastErrorCode();
            globed::netLog("(E) getaddrinfo failed (code {}): {}", code, GLOBED_LAZY(util::net::lastErrorString(code, true)));

            return Err(util::net::lastErrorString(code, true));
        }

        if (result->ai_family != util::net::activeAddressFamily() || result->ai_socktype != SOCK_DGRAM || result->ai_protocol != IPPROTO_UDP) {
            globed::netLog(
                "(E) getaddrinfo returned unexpected results: ai_family={}, ai_socktype={}, ai_protocol={}",
                result->ai_family, result->ai_socktype, result->ai_protocol
            );

            ::freeaddrinfo(result);
            return Err("getaddrinfo returned wrong output");
        }

        size_t toCopy = result->ai_addrlen;

        auto* ipaddr = (struct sockaddr_in*) result->ai_addr;

        std::memcpy(&out, ipaddr, toCopy);

        ::freeaddrinfo(result);

        return Ok();
    }

    Result<> getaddrinfoDoh(std::string_view hostname, sockaddr_storage& out) {
        globed::netLog("getaddrinfoDoh({})", hostname);

        auto af = util::net::activeAddressFamily();
        const char* family = (af == AF_INET) ? "A" : "AAAA";

        auto test = WebRequestManager::get().dnsOverHttps(hostname, family);

        while (test.isPending()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        if (test.isCancelled()) {
            return Err("DNS over HTTPS request was cancelled");
        }

        auto val = test.getFinishedValue();
        if (val->isErr()) {
            globed::netLog("(E) getaddrinfoDoh({}) failed: {}", hostname, val->unwrapErr());
            return Err(val->unwrapErr());
        }

        auto ip = std::move(val)->unwrap();

        if (af == AF_INET) {
            return stringToInAddr(ip.c_str(), reinterpret_cast<sockaddr_in&>(out).sin_addr);
        } else {
            return stringToInAddr(ip.c_str(), reinterpret_cast<sockaddr_in6&>(out).sin6_addr);
        }
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

    Result<std::string> inAddrToString(const in6_addr& addr) {
        std::string out;
        out.resize(46);

        auto ntopResult = inet_ntop(AF_INET6, &addr, out.data(), 46);

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

    Result<> stringToInAddr(const char* addr, in6_addr& out) {
        auto ptonResult = inet_pton(AF_INET6, addr, &out);

        if (ptonResult > 0) {
            return Ok();
        } else {
            return Err(lastErrorString());
        }
    }

    uint16_t hostToNetworkPort(uint16_t port) {
        return util::data::byteswap(port);
    }

    int activeAddressFamily() {
        return GlobedSettings::get().launchArgs().useIpv6 ? AF_INET6 : AF_INET;
    }

    size_t activeAddressFamilySize() {
        return GlobedSettings::get().launchArgs().useIpv6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);
    }
}

