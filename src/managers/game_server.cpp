#include "game_server.hpp"

#include <util/net.hpp>
#include <util/rng.hpp>
#include <util/collections.hpp>

void GameServerManager::addServer(const std::string_view serverId, const std::string_view name, const std::string_view address, const std::string_view region) {
    auto addr = util::net::splitAddress(address, DEFAULT_PORT);
    GameServer server = {
        .id = std::string(serverId),
        .name = std::string(name),
        .region = std::string(region),
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
    data->servers.emplace(serverId, gsdata);
}

void GameServerManager::clear() {
    _data.lock()->servers.clear();
}

size_t GameServerManager::count() {
    return _data.lock()->servers.size();
}

void GameServerManager::setActive(const std::string_view id) {
    auto data = _data.lock();
    data->active = id;

    this->saveLastConnected(id);
}

void GameServerManager::clearActive() {
    auto data = _data.lock();
    data->active.clear();

    this->saveLastConnected("");
}

std::string GameServerManager::getActiveId() {
    return _data.lock()->active;
}

std::optional<GameServer> GameServerManager::getActiveServer() {
    auto active = this->getActiveId();
    if (active.empty()) {
        return std::nullopt;
    }

    return this->getServer(active);
}

GameServer GameServerManager::getServer(const std::string_view id_) {
    std::string id(id_);

    auto data = _data.lock();
    if (!data->servers.contains(id)) {
        throw(std::runtime_error(std::string("invalid server ID, no such server exists: ") + id));
    }

    return data->servers.at(id).server;
}

std::unordered_map<std::string, GameServer> GameServerManager::getAllServers() {
    auto data = _data.lock();

    std::unordered_map<std::string, GameServer> out;

    for (const auto& [_, gsd] : data->servers) {
        out.insert(std::make_pair(gsd.server.id, gsd.server));
    }

    return out;
}

uint32_t GameServerManager::getActivePing() {
    auto server = this->getActiveServer();
    GLOBED_REQUIRE(server.has_value(), "tried to request ping of the active server when not connected to any")

    return server.value().ping;
}

void GameServerManager::saveStandalone(const std::string_view addr) {
    geode::Mod::get()->setSavedValue(STANDALONE_SETTING_KEY, std::string(addr));
}

std::string GameServerManager::loadStandalone() {
    return geode::Mod::get()->getSavedValue<std::string>(STANDALONE_SETTING_KEY);
}

void GameServerManager::saveLastConnected(const std::string_view addr) {
    geode::Mod::get()->setSavedValue(LAST_CONNECTED_SETTING_KEY, std::string(addr));
}

std::string GameServerManager::loadLastConnected() {
    return geode::Mod::get()->getSavedValue<std::string>(LAST_CONNECTED_SETTING_KEY);
}

uint32_t GameServerManager::startPing(const std::string_view serverId) {
    auto pingId = util::rng::Random::get().generate<uint32_t>();

    auto data = _data.lock();
    auto& gsdata = data->servers.at(std::string(serverId));

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

    for (auto& [_, server] : data->servers) {
        if (server.pendingPings.contains(pingId)) {
            auto start = server.pendingPings.at(pingId);
            auto timeTook = util::time::asMillis(now - start);

            server.server.ping = timeTook;
            server.server.playerCount = playerCount;
            server.pendingPings.erase(pingId);
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
