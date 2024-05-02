#include "resource.hpp"
#include "battery/embed.hpp"

#include <managers/error_queues.hpp>
#include <util/crypto.hpp>

using namespace geode::prelude;
using namespace util::data;

GlobedResourceManager::GlobedResourceManager() {
    std::string jerror;
    auto rresult = matjson::parse(b::embed<"baked-resources.json">().data(), jerror);

    if (!rresult) {
        log::warn("[ResourceManager] failed to load resources: {}", jerror);
        return;
    }

    auto result = std::move(static_cast<matjson::Value&&>(rresult.value()));

    if (!result.contains("strings") || !result["strings"].is_object()) {
        log::warn("[ResourceManager] file has no 'strings' key, not loading");
        return;
    }

    auto& arr = result["strings"].as_object();

    for (const auto& [key, value] : arr) {
        if (!value.is_string()) {
            log::warn("[ResourceManager] skipping non-string key: {}", key);
            continue;
        }

        this->addString(key, value.as_string());
    }
}

const std::string& GlobedResourceManager::getString(const std::string_view key) {
    uint32_t hashed = hash(key);
    return getStringByHash(hashed);
}

const std::string& GlobedResourceManager::getStringByHash(uint32_t hashed) {
#ifdef GLOBED_DEBUG
    if (!strings.contains(hashed)) {
        ErrorQueues::get().warn(fmt::format("attempted to get string with invalid key"));
    }
#endif

    auto& val = strings[hashed];
    if (val.empty()) {
        val = "<no string>";
    }

    return val;
}

void GlobedResourceManager::addString(const std::string_view key, const std::string_view value) {
    strings[hash(key)] = value;
}

uint32_t GlobedResourceManager::hash(const std::string_view key) {
    return util::crypto::adler32(reinterpret_cast<const byte*>(key.data()), key.size());
}

