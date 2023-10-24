#include "net.hpp"
#include <net/defs.hpp>
#include <Geode/Geode.hpp>

namespace util::net {
    void initialize() {
#ifdef GEODE_IS_WINDOWS
        WSADATA wsaData;
        bool success = WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
        if (!success) {
            throwLastError();
        }
#endif
    }

    void cleanup() {
#ifdef GEODE_IS_WINDOWS
        WSACleanup();
#endif
    }

    int lastErrorCode() {
#ifdef GEODE_IS_WINDOWS
        return WSAGetLastError();
#else
        return errno;
#endif
    }

    std::string lastErrorString() {
        return lastErrorString(lastErrorCode());
    }

    std::string lastErrorString(int code) {
#ifdef GEODE_IS_WINDOWS
    char *s = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&s, 0, NULL);
    std::string formatted = fmt::format("[Win error {}]: {}", code, s);
    LocalFree(s);
    return formatted;
#else
    return fmt::format("[Unix error {}]: {}", code, strerror(code));
#endif
    }

    void throwLastError() {
        auto message = lastErrorString();
        geode::log::warn("Throwing network exception: {}", message);
        throw std::runtime_error(std::string("Network error: ") + message);
    }
}