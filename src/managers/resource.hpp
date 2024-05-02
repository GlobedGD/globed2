#pragma once
#include <defs/util.hpp>

class GlobedResourceManager : public SingletonBase<GlobedResourceManager> {
    friend class SingletonBase;
    GlobedResourceManager();

public:
    const std::string& getString(const std::string_view key);
    const std::string& getStringByHash(uint32_t hash);

private:
    std::unordered_map<uint32_t, std::string> strings;

    void addString(const std::string_view key, const std::string_view value);
    uint32_t hash(const std::string_view key);
};
