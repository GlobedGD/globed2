#pragma once
#include <defs.hpp>
#include <util/sync.hpp>
#include <util/collections.hpp>
#include <util/time.hpp>

#include <unordered_map>

struct GameServerAddress {
    std::string ip;
    unsigned short port;
};

struct GameServerInfo {
    std::string name;
    std::string region;

    GameServerAddress address;

    int ping;
    std::unordered_map<uint32_t, chrono::milliseconds> pendingPings;
    util::collections::CappedQueue<chrono::milliseconds, 100> pingHistory;

    uint32_t playerCount;
};

// provides a view with a bit less data than GameServerInfo
struct GameServerView {
    std::string id;
    std::string name;
    std::string region;

    GameServerAddress address;

    int ping;
    uint32_t playerCount;
};

// This class is fully thread safe to use.
class GlobedServerManager {
    GLOBED_SINGLETON(GlobedServerManager);
    GlobedServerManager();

    /* central server control */

    void setCentral(std::string address);
    std::string getCentral();

    /* game servers */
    void addGameServer(const std::string& serverId, const std::string& name, const std::string& address, const std::string& region);
    void setActiveGameServer(const std::string& serverId);
    std::string getActiveGameServer();
    void clearGameServers();

    size_t gameServerCount();

    uint32_t pingStart(const std::string& serverId);
    // start a ping on the active game server
    void pingStartActive();

    void pingFinish(uint32_t pingId, uint32_t playerCount);
    // finish a ping on the active game server
    void pingFinishActive(uint32_t playerCount);

    GameServerView getGameServer(const std::string& serverId);
    std::vector<chrono::milliseconds> getPingHistory(const std::string& serverId);

    std::unordered_map<std::string, GameServerView> extractGameServers();

    util::sync::AtomicBool pendingChanges;

private:
    struct InnerData {
        std::unordered_map<std::string, GameServerInfo> servers;
        std::string central; // current central url
        std::string game; // current game server ID
        uint32_t activePingId;
    };

    util::sync::WrappingMutex<InnerData> _data;
};