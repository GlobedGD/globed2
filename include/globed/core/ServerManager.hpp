#pragma once

#include <globed/util/singleton.hpp>
#include <matjson.hpp>
#include <matjson/reflect.hpp>
#include <matjson/std.hpp>

namespace globed {

struct CentralServerData {
    std::string name;
    std::string url;
};

class GLOBED_DLL ServerManager : public SingletonBase<ServerManager> {
    friend class SingletonBase;
    ServerManager();

public:
    CentralServerData &getServer(size_t index);
    CentralServerData &getActiveServer();
    std::vector<CentralServerData> &getAllServers();
    size_t getActiveIndex();
    void switchTo(size_t index);
    void deleteServer(size_t index);
    void addServer(CentralServerData &&data);
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

} // namespace globed