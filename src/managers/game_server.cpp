#include "game_server.hpp"

#include <matjson/reflect.hpp>
#include <util/net.hpp>
#include <util/rng.hpp>
#include <util/collections.hpp>
#include <net/manager.hpp>
#include <data/types/misc.hpp>

using namespace geode::prelude;
using namespace asp::time;

GameServerManager::GameServerManager() {}

Result<> GameServerManager::addServer(std::string_view serverId, std::string_view name, std::string_view address, std::string_view region) {
    _data.lock()->servers.erase(std::string(serverId));
    GLOBED_UNWRAP(this->addOrUpdateServer(serverId, name, address, region));
    return Ok();
}

Result<bool> GameServerManager::addOrUpdateServer(std::string_view serverId_, std::string_view name, std::string_view address, std::string_view region) {
    auto serverId = std::string(serverId_);

    int ping = -1;
    uint16_t playerCount = 0;

    auto data = _data.lock();
    if (data->servers.contains(serverId)) {
        auto& server = data->servers.at(serverId).server;

        ping = server.ping;
        playerCount = server.playerCount;

        // check if the server changed
        if (
            server.id == serverId_
            && server.name == name
            && server.region == region
            && server.address == address
        ) {
            return Ok(false);
        }
    }

    GameServer server = {
        .id = std::string(serverId),
        .name = std::string(name),
        .region = std::string(region),
        .address = std::string(address),
        .ping = ping,
        .playerCount = playerCount,
    };

    GameServerManager::GameServerData gsdata = {
        .server = server
    };

    data->servers[serverId] = gsdata;

    return Ok(true);
}

void GameServerManager::addOrUpdateRelay(const ServerRelay& relay) {
    auto data = _data.lock();
    data->relays[relay.id] = relay;
}

void GameServerManager::reloadActiveRelay() {
    auto id = Mod::get()->getSavedValue<std::string>("active-server-relay");

    if (!id.empty()) {
        log::info("Reloading relay ({})", id);
    }

    this->setActiveRelay(id);
}

void GameServerManager::clear() {
    auto data = _data.lock();
    data->servers.clear();
    data->relays.clear();

    pendingChanges = true;
}

std::vector<ServerRelay> GameServerManager::getAllRelays() {
    std::vector<ServerRelay> out;

    for (auto& [k, v] : _data.lock()->relays) {
        out.push_back(v);
    }

    return out;
}

void GameServerManager::setActiveRelay(const std::string& id) {
    auto data = _data.lock();

    if (data->relays.contains(id)) {
        data->activeRelay = data->relays.at(id).id;
    } else {
        data->activeRelay = "";
    }

    Mod::get()->setSavedValue("active-server-relay", data->activeRelay);
}

std::string GameServerManager::getActiveRelayId() {
    return _data.lock()->activeRelay;
}

std::optional<ServerRelay> GameServerManager::getActiveRelay() {
    auto data = _data.lock();

    if (data->activeRelay.empty()) {
        return std::nullopt;
    }

    if (!data->relays.contains(data->activeRelay)) {
        data->activeRelay = "";
        return std::nullopt;
    }

    return data->relays.at(data->activeRelay);
}

size_t GameServerManager::count() {
    return _data.lock()->servers.size();
}

void GameServerManager::setActive(std::string_view id) {
    auto idstr = std::string(id);
    auto data = _data.lock();
    if (!data->servers.contains(idstr)) return;

    data->active = id;

    this->saveLastConnected(id);
}

void GameServerManager::clearActive() {
    auto data = _data.lock();
    data->active.clear();

    this->saveLastConnected("");
}

bool GameServerManager::hasRelays() {
    return this->relayCount();
}

size_t GameServerManager::relayCount() {
    return _data.lock()->relays.size();
}

void GameServerManager::clearAllExcept(std::string_view id) {
    auto data = _data.lock();

    auto key = std::string(id);
    auto value = data->servers.at(key);
    data->servers.clear();
    data->servers.emplace(std::move(key), std::move(value));
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

std::optional<GameServer> GameServerManager::getServer(std::string_view id_) {
    std::string id(id_);

    auto data = _data.lock();
    if (!data->servers.contains(id)) {
        return std::nullopt;
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

int GameServerManager::getActivePing() {
    auto server = this->getActiveServer();
    GLOBED_REQUIRE(server.has_value(), "tried to request ping of the active server when not connected to any")

    return server.value().ping;
}

int GameServerManager::getActivePlayerCount() {
    auto server = this->getActiveServer();

    return server ? server->playerCount : 0;
}

void GameServerManager::saveStandalone(std::string_view addr) {
    Mod::get()->setSavedValue(STANDALONE_SETTING_KEY, std::string(addr));
}

std::string GameServerManager::loadStandalone() {
    return Mod::get()->getSavedValue<std::string>(STANDALONE_SETTING_KEY);
}

void GameServerManager::saveLastConnected(std::string_view addr) {
    Mod::get()->setSavedValue(LAST_CONNECTED_SETTING_KEY, std::string(addr));
}

std::string GameServerManager::loadLastConnected() {
    return Mod::get()->getSavedValue<std::string>(LAST_CONNECTED_SETTING_KEY);
}

uint32_t GameServerManager::startPing(std::string_view serverId) {
    auto pingId = util::rng::Random::get().generate<uint32_t>();

    auto data = _data.lock();
    auto& gsdata = data->servers.at(std::string(serverId));

    if (gsdata.pendingPings.size() > 50) {
        log::warn("over 50 pending pings for the game server {}, clearing", serverId);
        gsdata.pendingPings.clear();
    }

    auto now = SystemTime::now();
    gsdata.pendingPings[pingId] = now;

    return pingId;
}

void GameServerManager::finishPing(uint32_t pingId, uint32_t playerCount) {
    auto data = _data.lock();

    for (auto& [_, server] : data->servers) {
        if (server.pendingPings.contains(pingId)) {
            auto start = server.pendingPings.at(pingId);
            auto timeTook = start.elapsed().millis();

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

void GameServerManager::backupInternalData() {
    *_dataBackup.lock() = *_data.lock();
}

void GameServerManager::restoreInternalData() {
    *_data.lock() = *_dataBackup.lock();
}
