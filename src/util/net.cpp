#include "net.hpp"
#include <defs.hpp>

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

    std::string lastErrorString() {
        return lastErrorString(lastErrorCode());
    }

    std::string lastErrorString(int code) {
#ifdef GLOBED_WIN32
    char *s = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&s, 0, NULL);
    std::string formatted = std::string("[Win error ") + std::to_string(code) + "]: " + std::string(s);
    LocalFree(s);
    return formatted;
#else
    return std::string("[Unix error ") + std::to_string(code) + "]: " + std::string(strerror(code));
#endif
    }

    void throwLastError() {
        auto message = lastErrorString();
        GLOBED_ASSERT_LOG(std::string("Throwing network exception: ") + message);
        throw std::runtime_error(std::string("Network error: ") + message);
    }
}