#include <globed/core/PlayerCacheManager.hpp>

using namespace geode::prelude;

namespace globed {

void PlayerCacheManager::insert(int accountId, const PlayerDisplayData &data)
{
    m_layer1[accountId] = Entry{data};

    if (m_layer2.contains(accountId)) {
        m_layer2.erase(accountId);
    }
}

void PlayerCacheManager::remove(int accountId)
{
    m_layer1.erase(accountId);
    m_layer2.erase(accountId);
}

std::optional<PlayerDisplayData> PlayerCacheManager::get(int accountId)
{
    auto it = m_layer1.find(accountId);

    if (it != m_layer1.end()) {
        return it->second.data;
    }

    auto it2 = m_layer2.find(accountId);
    if (it2 != m_layer2.end()) {
        return it2->second.data;
    }

    return std::nullopt;
}

PlayerDisplayData PlayerCacheManager::getOrDefault(int accountId)
{
    auto dataOpt = this->get(accountId);
    if (dataOpt.has_value()) {
        return dataOpt.value();
    }

    return DEFAULT_PLAYER_DATA;
}

bool PlayerCacheManager::has(int accountId)
{
    return m_layer1.contains(accountId) || m_layer2.contains(accountId);
}

bool PlayerCacheManager::hasInLayer1(int accountId)
{
    return m_layer1.contains(accountId);
}

void PlayerCacheManager::evictToLayer2(int accountId)
{
    auto it = m_layer1.find(accountId);

    if (it != m_layer1.end()) {
        m_layer2[accountId] = Entry{std::move(it->second.data)};
        m_layer1.erase(it);
    }
}

void PlayerCacheManager::evictAllToLayer2()
{
    for (auto &[accountId, entry] : m_layer1) {
        m_layer2[accountId] = Entry{std::move(entry.data)};
    }

    m_layer1.clear();
}

} // namespace globed
