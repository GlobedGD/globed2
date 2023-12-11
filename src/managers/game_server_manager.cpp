#include "game_server_manager.hpp"

#include <util/net.hpp>
#include <util/rng.hpp>
#include <util/collections.hpp>

GameServerManager::GameServerManager() {}

void GameServerManager::addServer(const std::string& serverId, const std::string& name, const std::string& address, const std::string& region) {
    auto addr = util::net::splitAddress(address, 41001);
    GameServer server = {
        .id = serverId,
        .name = name,
        .region = region,
        .address = GameServerAddress {
            .ip = addr.first,
            .port = addr.second
        },
        .ping = -1,
        .playerCount = 0,
    };

    GameServerManager::GameServerData gsdata = {
        .server = server
    };

    auto data = _data.lock();
    data->servers[serverId] = gsdata;
}

void GameServerManager::clear() {
    _data.lock()->servers.clear();
}

size_t GameServerManager::count() {
    return _data.lock()->servers.size();
}

void GameServerManager::setActive(const std::string& id) {
    auto data = _data.lock();
    data->active = id;
}

void GameServerManager::clearActive() {
    auto data = _data.lock();
    data->active.clear();
}

std::string GameServerManager::active() {
    return _data.lock()->active;
}

std::optional<GameServer> GameServerManager::getActiveServer() {
    auto active = this->active();
    if (active.empty()) {
        return std::nullopt;
    }

    return this->getServer(active);
}

GameServer GameServerManager::getServer(const std::string& id) {
    auto data = _data.lock();
    if (!data->servers.contains(id)) {
        THROW(std::runtime_error(std::string("invalid server ID, no such server exists: ") + id));
    }

    return data->servers.at(id).server;
}

std::unordered_map<std::string, GameServer> GameServerManager::getAllServers() {
    auto data = _data.lock();
    auto values = util::collections::mapValuesBorrowed(data->servers);

    std::unordered_map<std::string, GameServer> out;

    for (auto* gsd : values) {
        out.insert(std::make_pair(gsd->server.id, gsd->server));
    }

    return out;
}


uint32_t GameServerManager::startPing(const std::string& serverId) {
    auto pingId = util::rng::Random::get().generate<uint32_t>();

    auto data = _data.lock();
    auto& gsdata = data->servers.at(serverId);

    if (gsdata.pendingPings.size() > 50) {
        geode::log::warn("over 50 pending pings for the game server {}, clearing", serverId);
        gsdata.pendingPings.clear();
    }

    auto now = util::time::now();
    gsdata.pendingPings[pingId] = now;

    return pingId;
}

void GameServerManager::finishPing(uint32_t pingId, uint32_t playerCount) {
    auto now = util::time::now();

    auto data = _data.lock();

    for (auto* server : util::collections::mapValuesBorrowed(data->servers)) {
        if (server->pendingPings.contains(pingId)) {
            auto start = server->pendingPings.at(pingId);
            auto timeTook = util::time::asMillis(now - start);

            server->server.ping = timeTook;
            server->server.playerCount = playerCount;
            server->pendingPings.erase(pingId);
            return;
        }
    }
}

void GameServerManager::startKeepalive() {
    std::string active = _data.lock()->active;

    if (!active.empty()) {
        auto pingId = this->startPing(active);
        _data.lock()->activePingId = pingId;
    }
}

void GameServerManager::finishKeepalive(uint32_t playerCount) {
    uint32_t activePingId = _data.lock()->activePingId;
    this->finishPing(activePingId, playerCount);
}
