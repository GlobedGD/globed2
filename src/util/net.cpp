#include "net.hpp"

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
        char *s = nullptr;
        if (FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&s, 0, nullptr)
        == 0) {
            // some errors like WSA 10038 can raise ERROR_MR_MID_NOT_FOUND (0x13D)
            // which basically means the formatted message txt doesn't exist in the OS.
            // i call this a windows moment because like what the fuck???
            auto le = GetLastError();
            geode::log::error("FormatMessageA failed formatting error code {}, last error: {}", code, le);
            return std::string("[Unknown windows error ") + std::to_string(code) + "]: formatting failed because of: " + std::to_string(le);
        }

        std::string formatted = std::string("[Win error ") + std::to_string(code) + "]: " + std::string(s);
        LocalFree(s);
        return formatted;
#else
        return std::string("[Unix error ") + std::to_string(code) + "]: " + std::string(strerror(code));
#endif
    }

    [[noreturn]] void throwLastError() {
        auto message = lastErrorString();
        geode::log::error(std::string("Throwing network exception: ") + message);
        THROW(std::runtime_error(std::string("Network error: ") + message));
    }

    std::string webUserAgent() {
        return fmt::format("globed-geode-xd/{}", geode::Mod::get()->getVersion().toString());
    }

    std::pair<std::string, unsigned short> splitAddress(const std::string& address, unsigned short defaultPort) {
        std::pair<std::string, unsigned short> out;

        size_t colon = address.find(':');
        bool hasPort = colon != std::string::npos && colon != 0;

        GLOBED_REQUIRE(hasPort || defaultPort != 0, "invalid address, cannot split into IP and port")

        if (hasPort) {
            out.first = address.substr(0, colon);
            std::istringstream portStream(address.substr(colon + 1));
            if (!(portStream >> out.second)) {
                // if no default port - fail, otherwise use it
                GLOBED_REQUIRE(defaultPort != 0, "invalid port number")
                out.second = defaultPort;
            }
        } else {
            out.first = address;
            out.second = defaultPort;
        }

        return out;
    }
}