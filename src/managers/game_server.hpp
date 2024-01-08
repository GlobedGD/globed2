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
    constexpr static const char* STANDALONE_SETTING_KEY = "_last-standalone-addr";

    util::sync::AtomicBool pendingChanges;

    void addServer(const std::string_view serverId, const std::string_view name, const std::string_view address, const std::string_view region);
    void clear();
    size_t count();

    void setActive(const std::string_view id);
    std::string getActiveId();
    void clearActive();

    std::optional<GameServer> getActiveServer();
    GameServer getServer(const std::string_view id);
    std::unordered_map<std::string, GameServer> getAllServers();

    // return ping on the active server
    uint32_t getActivePing();

    // save the given address as a last connected standalone address
    void saveStandalone(const std::string_view addr);
    std::string loadStandalone();

    /* pings */

    uint32_t startPing(const std::string_view serverId);
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