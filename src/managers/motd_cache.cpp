#include "motd_cache.hpp"

#include <managers/central_server.hpp>

void MotdCacheManager::insert(std::string serverUrl, std::string motd, std::string motdHash) {
    entries[std::move(serverUrl)] = Entry {
        .motd = std::move(motd),
        .hash = std::move(motdHash)
    };
}

void MotdCacheManager::insertActive(std::string motd, std::string motdHash) {
    auto& csm = CentralServerManager::get();
    auto server = csm.getActive();

    if (!server) {
        return;
    }

    this->insert(std::move(server->url), std::move(motd), std::move(motdHash));
}

std::optional<std::string> MotdCacheManager::getMotd(std::string serverUrl) {
    if (entries.contains(serverUrl)) {
        return entries.at(serverUrl).motd;
    }

    return std::nullopt;
}

std::optional<std::string> MotdCacheManager::getCurrentMotd() {
    auto& csm = CentralServerManager::get();
    auto server = csm.getActive();

    if (!server) {
        return std::nullopt;
    }

    return this->getMotd(server->url);
}
