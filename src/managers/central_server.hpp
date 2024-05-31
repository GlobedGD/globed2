#pragma once

#include <asp/sync.hpp> // mutex

#include <data/bytebuffer.hpp>
#include <util/crypto.hpp> // base64
#include <util/singleton.hpp>

struct CentralServer {
    std::string name;
    std::string url;

};

GLOBED_SERIALIZABLE_STRUCT(CentralServer, (name, url));
GLOBED_VERIFY_SERIALIZED_STRUCT(CentralServer);

class CentralServerManager : public SingletonBase<CentralServerManager> {
protected:
    friend class SingletonBase;
    CentralServerManager();

public:
    // set the current active server, thread safe
    void setActive(int index);
    // get the current active server, thread safe
    std::optional<CentralServer> getActive();
    int getActiveIndex();

    // get all central servers, thread safe
    std::vector<CentralServer> getAllServers();
    // get a central server at index, thread safe
    CentralServer getServer(int index);

    // set the current active server to an emulated central server instance for a standalone game server
    void setStandalone(bool status = true);
    bool standalone();

    // get the total amount of servers
    size_t count();

    // add a new central server and save. only call from main thread
    void addServer(const CentralServer& data);
    // remove a central server and save. only call from main thread
    void removeServer(int index);
    // modify a central server and save. only call from main thread
    void modifyServer(int index, const CentralServer& data);

    // reload all saved central servers, only call from main thread
    void reload();

    // clear the active authtoken, reinitialize account manager, clear game servers, and switch to a central server by its ID
    void switchRoutine(int index, bool force = false);

    asp::AtomicBool recentlySwitched = false;

protected:
    asp::Mutex<std::vector<CentralServer>> _servers;
    asp::AtomicI32 _activeIdx = -1;

    constexpr static const char* SETTING_KEY = "_central-server-list";
    constexpr static const char* ACTIVE_SERVER_KEY = "_central-server-active";
    constexpr static int STANDALONE_IDX = -2;

    void save();
};