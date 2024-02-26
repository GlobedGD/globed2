#include <util/net.hpp>
#include <util/format.hpp>

void util::net::initialize() {
    WSADATA wsaData;
    bool success = WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    if (!success) {
        throwLastError();
    }
}

void util::net::cleanup() {
    WSACleanup();
}

int util::net::lastErrorCode() {
    return WSAGetLastError();
}

std::string util::net::lastErrorString(int code, bool gai) {
    char *s = nullptr;
    if (FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&s, 0, nullptr)
    == 0) {
        // some errors like WSA 10038 can raise ERROR_MR_MID_NOT_FOUND (0x13D)
        // which basically means the formatted message txt doesn't exist in the OS.
        auto le = GetLastError();
        log::error("FormatMessageA failed formatting error code {}, last error: {}", code, le);
        return fmt::format("[Unknown windows error {}]: formatting failed because of: {}", code, le);
    }

    std::string formatted = fmt::format("[Win error {}]: {}", code, util::format::trim(s));
    LocalFree(s);
    return formatted;
}