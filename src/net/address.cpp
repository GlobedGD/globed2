#include "address.hpp"

#ifdef GEODE_IS_WINDOWS
# include <WS2tcpip.h>
#else
# include <netinet/in.h>
#endif

#include <managers/settings.hpp>
#include <util/format.hpp>
#include <util/net.hpp>

NetworkAddress::NetworkAddress() {
    this->set("", DEFAULT_PORT);
}

NetworkAddress::NetworkAddress(std::string_view address) {
    this->set(address);
}

NetworkAddress::NetworkAddress(std::string_view host, uint16_t port) {
    this->set(host, port);
}

void NetworkAddress::set(std::string_view address) {
    auto colon = address.find(':');
    if (colon == std::string::npos) {
        this->set(address, DEFAULT_PORT);
    } else {
        uint16_t port = util::format::parse<uint16_t>(address.substr(colon + 1)).value_or(DEFAULT_PORT);
        this->set(address.substr(0, colon), port);
    }
}

void NetworkAddress::set(std::string_view host, uint16_t port) {
    this->host = host;
    this->port = port;
}

std::string NetworkAddress::toString() const {
    return host + ":" + std::to_string(port);
}

Result<sockaddr_in> NetworkAddress::resolve() const {
    globed::netLog("Resolving host {}", host);

    if (host.empty()) {
        return Err("empty IP address or domain name, cannot resolve");
    }

    // if it's cached then just use our cache
    if (dnsCache.contains(host)) {
        sockaddr_in out;
        out.sin_family = AF_INET;
        out.sin_port = util::net::hostToNetworkPort(port);
        out.sin_addr = dnsCache.at(host);

        // TODO This code causes clang to crash, uncomment when they fix it :)
        // https://github.com/llvm/llvm-project/issues/138428

        // globed::netLog(
        //     "Host was cached, returning '{}'",
        //     GLOBED_LAZY(util::net::inAddrToString(out.sin_addr).unwrapOrElse([] { return "<error stringifying>"; }))
        // );

        auto doConvert = [&] {
            return util::net::inAddrToString(out.sin_addr).unwrapOrElse([] { return "<error stringifying>"; });
        };

        globed::netLog(
            "Host was cached, returning '{}'",
            GLOBED_LAZY(doConvert())
        );

        return Ok(out);
    }

    // for some reason this must be heap allocated or windows complains
    auto addr = std::make_unique<sockaddr_in>();
    addr->sin_family = AF_INET;

    auto ipResult = util::net::stringToInAddr(host.c_str(), addr->sin_addr);

    // if this is not a valid IP address, try getaddrinfo
    if (!ipResult) {
        GLOBED_UNWRAP(util::net::getaddrinfo(host, *addr));
    }

    // TODO: same as the todo above
    // globed::netLog(
    //     "Adding host to DNS cache ('{}' -> '{}')",
    //     host,
    //     GLOBED_LAZY(util::net::inAddrToString(addr->sin_addr).unwrapOrElse([] { return "<error stringifying>"; }))
    // );

    auto doConvert = [&] {
        return util::net::inAddrToString(addr->sin_addr).unwrapOrElse([] { return "<error stringifying>"; });
    };

    globed::netLog(
        "Adding host to DNS cache ('{}' -> '{}')",
        host, GLOBED_LAZY(doConvert())
    );

    // add to cache
    dnsCache.emplace(std::make_pair(host, addr->sin_addr));

    sockaddr_in copy;
    std::memcpy(&copy, addr.get(), sizeof(sockaddr_in));
    copy.sin_family = AF_INET;
    copy.sin_port = util::net::hostToNetworkPort(port);
    return Ok(copy);
}

Result<std::string> NetworkAddress::resolveToString() const {
    // resolve to a sockaddr_in first
    GLOBED_UNWRAP_INTO(this->resolve(), auto addr);

    // convert to string
    GLOBED_UNWRAP_INTO(util::net::inAddrToString(addr.sin_addr), auto ipstr);

    return Ok(std::move(ipstr + ":" + std::to_string(port)));
}

bool NetworkAddress::isEmpty() const {
    return host.empty();
}
