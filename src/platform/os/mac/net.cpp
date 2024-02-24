#include <util/net.hpp>

void util::net::initialize() {}
void util::net::cleanup() {}

int util::net::lastErrorCode() {
    return errno;
}

std::string util::net::lastErrorString(int code, bool gai) {
    if (gai) return fmt::format("[Unix gai error {}]: {}", code, gai_strerror(code));
    return fmt::format("[Unix error {}]: {}", code, strerror(code));
}
