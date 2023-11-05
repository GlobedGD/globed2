#pragma once
#include <defs.hpp>
#include <util/sync.hpp>
#include <util/collections.hpp>

#include <unordered_map>
#include <chrono>

struct GameServerAddress {
    std::string ip;
    unsigned short port;
};

struct GameServerInfo {
    GameServerAddress address;

    std::chrono::milliseconds ping;
    std::unordered_map<uint32_t, std::chrono::milliseconds> pendingPings;
    util::collections::CappedQueue<std::chrono::milliseconds, 100> pingHistory;

    uint32_t playerCount;
};

// provides a view with server ping and player count
struct GameServerView {
    std::chrono::milliseconds ping;
    uint32_t playerCount;
};

class GameServerManager {
public:
    GLOBED_SINGLETON(GameServerManager);
    GameServerManager();

    uint32_t addPendingPing(const std::string& serverId);
    void recordPingResponse(uint32_t pingId, uint32_t playerCount);

    GameServerView getServerView(const std::string& serverId);
    std::vector<std::chrono::milliseconds> getPingHistory(const std::string& serverId);

    std::unordered_map<std::string, GameServerAddress> getServerAddresses();

private:
    util::sync::WrappingRwLock<std::unordered_map<std::string, GameServerInfo>> servers;
};