#pragma once

#include <util/singleton.hpp>

class MotdCacheManager : public SingletonBase<MotdCacheManager> {
    friend class SingletonBase;

public:
    void insert(std::string serverUrl, std::string motd, std::string motdHash);
    void insertActive(std::string motd, std::string motdHash);
    std::optional<std::string> getMotd(std::string serverUrl);
    std::optional<std::string> getCurrentMotd();

private:
    struct Entry {
        std::string motd;
        std::string hash;
    };

    std::unordered_map<std::string, Entry> entries; // server url to Entry
};