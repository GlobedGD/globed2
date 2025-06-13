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
    auto colon = address.rfind(':');
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

Result<sockaddr_storage> NetworkAddress::resolve() const {
    globed::netLog("Resolving host {}", host);

    if (host.empty()) {
        return Err("empty IP address or domain name, cannot resolve");
    }

    // if it's cached then just use our cache
    if (dnsCache.contains(host)) {
        auto& cachedEnt = dnsCache.at(host);

        sockaddr_storage out;
        out.ss_family = util::net::activeAddressFamily();

        if (util::net::activeAddressFamily() == AF_INET) {
            auto& outAddr = reinterpret_cast<sockaddr_in&>(out);

            outAddr.sin_port = util::net::hostToNetworkPort(port);
            outAddr.sin_addr = reinterpret_cast<sockaddr_in&>(cachedEnt).sin_addr;
        } else {
            auto& outAddr = reinterpret_cast<sockaddr_in6&>(out);
            outAddr.sin6_port = util::net::hostToNetworkPort(port);
            outAddr.sin6_addr = reinterpret_cast<sockaddr_in6&>(cachedEnt).sin6_addr;
        }

        // TODO This code causes clang to crash, uncomment when they fix it :)
        // https://github.com/llvm/llvm-project/issues/138428

        // globed::netLog(
        //     "Host was cached, returning '{}'",
        //     GLOBED_LAZY(util::net::inAddrToString(out.sin_addr).unwrapOrElse([] { return "<error stringifying>"; }))
        // );

        auto doConvert = [&] {
            if (util::net::activeAddressFamily() == AF_INET6) {
                return util::net::inAddrToString(reinterpret_cast<sockaddr_in6&>(out).sin6_addr).unwrapOrElse([] { return "<error stringifying>"; });
            } else {
                return util::net::inAddrToString(reinterpret_cast<sockaddr_in&>(out).sin_addr).unwrapOrElse([] { return "<error stringifying>"; });
            }
        };

        globed::netLog(
            "Host was cached, returning '{}'",
            GLOBED_LAZY(doConvert())
        );

        return Ok(out);
    }

    // for some reason this must be heap allocated or windows complains
    auto addr = std::make_unique<sockaddr_storage>();
    addr->ss_family = util::net::activeAddressFamily();

    bool isIp = false;

    if (util::net::activeAddressFamily() == AF_INET) {
        auto& addr4 = reinterpret_cast<sockaddr_in&>(*addr);
        isIp = util::net::stringToInAddr(host.c_str(), addr4.sin_addr).isOk();
    } else {
        auto& addr6 = reinterpret_cast<sockaddr_in6&>(*addr);
        isIp = util::net::stringToInAddr(host.c_str(), addr6.sin6_addr).isOk();
    }

    // if this is not a valid IP address, try getaddrinfo
    if (!isIp) {
        if (GlobedSettings::get().globed.dnsOverHttps) {
            GLOBED_UNWRAP(util::net::getaddrinfoDoh(host, *addr));
        } else {
            GLOBED_UNWRAP(util::net::getaddrinfo(host, *addr));
        }
    }

    // TODO: same as the todo above
    // globed::netLog(
    //     "Adding host to DNS cache ('{}' -> '{}')",
    //     host,
    //     GLOBED_LAZY(util::net::inAddrToString(addr->sin_addr).unwrapOrElse([] { return "<error stringifying>"; }))
    // );

    auto doConvert = [&] {
        if (util::net::activeAddressFamily() == AF_INET6) {
            return util::net::inAddrToString(reinterpret_cast<sockaddr_in6&>(*addr).sin6_addr).unwrapOrElse([] { return "<error stringifying>"; });
        } else {
            return util::net::inAddrToString(reinterpret_cast<sockaddr_in&>(*addr).sin_addr).unwrapOrElse([] { return "<error stringifying>"; });
        }
    };

    globed::netLog(
        "Adding host to DNS cache ('{}' -> '{}')",
        host, GLOBED_LAZY(doConvert())
    );

    // add to cache
    dnsCache.emplace(std::make_pair(host, *addr));

    sockaddr_storage copy;
    std::memcpy(&copy, addr.get(), sizeof(sockaddr_storage));
    copy.ss_family = util::net::activeAddressFamily();

    if (copy.ss_family == AF_INET) {
        auto& addr4 = reinterpret_cast<sockaddr_in&>(copy);
        addr4.sin_port = util::net::hostToNetworkPort(port);
    } else {
        auto& addr6 = reinterpret_cast<sockaddr_in6&>(copy);
        addr6.sin6_port = util::net::hostToNetworkPort(port);
    }

    return Ok(copy);
}

Result<std::string> NetworkAddress::resolveToString() const {
    // resolve to a sockaddr_in first
    GLOBED_UNWRAP_INTO(this->resolve(), auto addr);

    // convert to string
    std::string ipstr;
    if (addr.ss_family == AF_INET) {
        GLOBED_UNWRAP_INTO(util::net::inAddrToString(reinterpret_cast<sockaddr_in&>(addr).sin_addr), ipstr);
    } else {
        GLOBED_UNWRAP_INTO(util::net::inAddrToString(reinterpret_cast<sockaddr_in6&>(addr).sin6_addr), ipstr);
    }

    return Ok(std::move(ipstr + ":" + std::to_string(port)));
}

bool NetworkAddress::isEmpty() const {
    return host.empty();
}
