#include "address.hpp"

#include <WS2tcpip.h>

#include <util/format.hpp>
#include <util/net.hpp>

NetworkAddress::NetworkAddress() {
    this->set("", DEFAULT_PORT);
}

NetworkAddress::NetworkAddress(const std::string_view address) {
    this->set(address);
}

NetworkAddress::NetworkAddress(const std::string_view ip, uint16_t port) {
    this->set(ip, port);
}

void NetworkAddress::set(const std::string_view address) {
    auto colon = address.find(':');
    if (colon == std::string::npos) {
        this->set(address, DEFAULT_PORT);
    } else {
        uint16_t port = util::format::parse<uint16_t>(address.substr(colon + 1)).value_or(DEFAULT_PORT);
        this->set(address.substr(0, colon), port);
    }
}

void NetworkAddress::set(const std::string_view ip, uint16_t port) {
    this->ip = ip;
    this->port = port;
}

std::string NetworkAddress::toString() const {
    return ip + ":" + std::to_string(port);
}

Result<sockaddr_in> NetworkAddress::resolve() const {
    if (ip.empty()) {
        return Err("empty IP address or domain name, cannot resolve");
    }

    // if it's cached then just use our cache
    if (gaiCache.contains(ip)) {
        sockaddr_in out;
        out.sin_family = AF_INET;
        out.sin_port = htons(port);
        out.sin_addr = gaiCache.at(ip);
        return Ok(out);
    }

    // for some reason this must be heap allocated or windows complains
    auto addr = std::make_unique<sockaddr_in>();

    bool validIp = (inet_pton(AF_INET, ip.c_str(), &addr->sin_addr) > 0);

    // if this is not a valid IP address, try getaddrinfo
    if (!validIp) {
        GLOBED_UNWRAP(util::net::getaddrinfo(ip, *addr));
    }

    // add to cache
    gaiCache.emplace(std::make_pair(ip, addr->sin_addr));

    sockaddr_in copy;
    std::memcpy(&copy, addr.get(), sizeof(sockaddr_in));
    copy.sin_family = AF_INET;
    copy.sin_port = htons(port);
    return Ok(copy);
}

Result<std::string> NetworkAddress::resolveToString() const {
    // resolve to a sockaddr_in first
    GLOBED_UNWRAP_INTO(this->resolve(), auto addr);

    // convert to string
    GLOBED_UNWRAP_INTO(util::net::inAddrToString(addr.sin_addr), auto ipstr);

    return Ok(std::move(ipstr + ":" + std::to_string(port)));
}
