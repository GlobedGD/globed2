#pragma once
#include <defs.hpp>

#include <unordered_map>

#include <util/sync.hpp> // mutex
#include <util/crypto.hpp> // base64
#include <util/time.hpp>

struct GameServerAddress {
    std::string ip;
    unsigned short port;
};

struct GameServer {
    std::string id;
    std::string name;
    std::string region;

    GameServerAddress address;

    int ping;
    uint32_t playerCount;
};

// This class is fully thread safe to use.
class GameServerManager : GLOBED_SINGLETON(GameServerManager) {
public:
    GameServerManager();

    constexpr static const char* STANDALONE_ID = "__standalone__server_id__";

    util::sync::AtomicBool pendingChanges;

    void addServer(const std::string& serverId, const std::string& name, const std::string& address, const std::string& region);
    void clear();
    size_t count();

    void setActive(const std::string& id);
    std::string active();
    void clearActive();

    std::optional<GameServer> getActiveServer();
    GameServer getServer(const std::string& id);
    std::unordered_map<std::string, GameServer> getAllServers();

    /* pings */

    uint32_t startPing(const std::string& serverId);
    void finishPing(uint32_t pingId, uint32_t playerCount);

    void startKeepalive();
    void finishKeepalive(uint32_t playerCount);

protected:
    // expansion of GameServer with pending pings
    struct GameServerData {
        GameServer server;
        std::unordered_map<uint32_t, util::time::time_point> pendingPings;
    };

    struct InnerData {
        std::unordered_map<std::string, GameServerData> servers;
        std::string active; // current game server ID
        uint32_t activePingId;
    };

    util::sync::WrappingMutex<InnerData> _data;
};