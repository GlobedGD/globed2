#pragma once

#include <matjson.hpp>
#include <matjson/reflect.hpp>
#include <matjson/std.hpp>
#include <globed/util/singleton.hpp>

namespace globed {

struct CentralServerData {
    std::string name;
    std::string url;
};

class ServerManager : public SingletonBase<ServerManager> {
    friend class SingletonBase;
    ServerManager();

public:
    CentralServerData& getServer(size_t index);
    CentralServerData& getActiveServer();
    std::vector<CentralServerData>& getAllServers();
    size_t getActiveIndex();
    void switchTo(size_t index);
    void deleteServer(size_t index);
    void addServer(CentralServerData&& data);
    void commit();

    bool isOfficialServerActive();

private:
    struct Storage {
        std::vector<CentralServerData> servers;
        size_t activeIdx = 0;
    };

    Storage m_storage;

    void reload();
};

}