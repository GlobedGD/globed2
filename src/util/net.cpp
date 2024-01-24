#include "net.hpp"

#include <util/formatting.hpp>

namespace util::net {
    void initialize() {
#ifdef GLOBED_WIN32
        WSADATA wsaData;
        bool success = WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
        if (!success) {
            throwLastError();
        }
#endif
    }

    void cleanup() {
#ifdef GLOBED_WIN32
        WSACleanup();
#endif
    }

    int lastErrorCode() {
#ifdef GLOBED_WIN32
        return WSAGetLastError();
#else
        return errno;
#endif
    }

    std::string lastErrorString(bool gai) {
        return lastErrorString(lastErrorCode(), gai);
    }

    std::string lastErrorString(int code, bool gai) {
#ifdef GLOBED_WIN32
        char *s = nullptr;
        if (FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&s, 0, nullptr)
        == 0) {
            // some errors like WSA 10038 can raise ERROR_MR_MID_NOT_FOUND (0x13D)
            // which basically means the formatted message txt doesn't exist in the OS. (wine issue?)
            auto le = GetLastError();
            geode::log::error("FormatMessageA failed formatting error code {}, last error: {}", code, le);
            return fmt::format("[Unknown windows error {}]: formatting failed because of: {}", code, le);
        }

        std::string formatted = fmt::format("[Win error {}]: {}", code, s);
        LocalFree(s);
        return formatted;
#else
        if (gai) return fmt::format("[Unix gai error {}]: {}", code, gai_strerror(code));
        return fmt::format("[Unix error {}]: {}", code, strerror(code));
#endif
    }

    [[noreturn]] void throwLastError(bool gai) {
        auto message = lastErrorString(gai);
        geode::log::error("Throwing network exception: {}", message);
        throw(std::runtime_error(std::string("Network error: ") + message));
    }

    std::string webUserAgent() {
        return fmt::format("globed-geode-xd/{}", geode::Mod::get()->getVersion().toString());
    }

    std::pair<std::string, unsigned short> splitAddress(const std::string_view address, unsigned short defaultPort) {
        std::pair<std::string, unsigned short> out;

        size_t colon = address.find(':');
        bool hasPort = colon != std::string::npos && colon != 0;

        GLOBED_REQUIRE(hasPort || defaultPort != 0, "invalid address, cannot split into IP and port")

        if (hasPort) {
            out.first = address.substr(0, colon);
            auto portResult = util::formatting::parse<unsigned short>(address.substr(colon + 1));

            if (!portResult.has_value() && defaultPort == 0) {
                GLOBED_REQUIRE(false, "invalid port number")
            }

            out.second = portResult.value_or(defaultPort);
        } else {
            out.first = address;
            out.second = defaultPort;
        }

        return out;
    }

    bool sameSockaddr(const sockaddr_in& s1, const sockaddr_in& s2) {
        if (s1.sin_family != s2.sin_family || s1.sin_port != s2.sin_port) {
            return false;
        }

        return std::memcmp(&s1.sin_addr, &s2.sin_addr, sizeof(s1.sin_addr)) == 0;
    }

    std::string getaddrinfo(const std::string_view hostname) {
        // TODO i have no fucking clue how this compiles on both windows and unix but i'm still not sure if this behaves as intended so please test it
        struct addrinfo hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        struct addrinfo* result;

        if (0 != ::getaddrinfo(hostname.data(), nullptr, &hints, &result)) {
            util::net::throwLastError(true);
        }

        GLOBED_REQUIRE(result->ai_family == AF_INET, "getaddrinfo returned invalid address family")
        GLOBED_REQUIRE(result->ai_socktype == SOCK_DGRAM, "getaddrinfo returned invalid socket type")
        GLOBED_REQUIRE(result->ai_protocol == IPPROTO_UDP, "getaddrinfo returned invalid ip protocol")
        auto* ipaddr = (struct sockaddr_in*) result->ai_addr;

        std::string out;
        out.resize(16);

        inet_ntop(AF_INET, &ipaddr->sin_addr, out.data(), 16);

        ::freeaddrinfo(result);
        return out;
    }
}

