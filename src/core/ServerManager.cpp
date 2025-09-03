#include <globed/core/ServerManager.hpp>
#include <globed/core/ValueManager.hpp>

using namespace geode::prelude;

namespace globed {

ServerManager::ServerManager() {
    this->reload();
}

CentralServerData& ServerManager::getServer(size_t index) {
    return m_storage.servers.at(index);
}

CentralServerData& ServerManager::getActiveServer() {
    return this->getServer(m_storage.activeIdx);
}

size_t ServerManager::getActiveIndex() {
    return m_storage.activeIdx;
}

void ServerManager::switchTo(size_t index) {
    if (index >= m_storage.servers.size()) return;

    m_storage.activeIdx = index;
    this->commit();
}

void ServerManager::deleteServer(size_t index) {
    if (index >= m_storage.servers.size()) return;

    m_storage.servers.erase(m_storage.servers.begin() + index);

    if (m_storage.activeIdx == index) {
        m_storage.activeIdx = 0;
    }

    this->commit();
}

void ServerManager::addServer(CentralServerData&& data) {
    m_storage.servers.push_back(std::move(data));
    this->commit();
}

std::vector<CentralServerData>& ServerManager::getAllServers() {
    return m_storage.servers;
}

void ServerManager::commit() {
    globed::setValue("core.central-servers", m_storage);
}

void ServerManager::reload() {
    if (auto opt = globed::value<Storage>("core.central-servers")) {
        m_storage = *opt;
    } else {
        m_storage = {};
    }

    // ensure the main server is always present and is the first one
    // TODO: let gdps override this
#ifdef GLOBED_MODULE_TCD_HNS
    constexpr static auto mainServerUrl = "qunet://15.204.223.9:6010";
#else
    constexpr static auto mainServerUrl = "qunet://globed.dev";
#endif

    auto it = std::find_if(
        m_storage.servers.begin(), m_storage.servers.end(),
        [](const CentralServerData& server) { return server.url == mainServerUrl; }
    );

    if (it == m_storage.servers.end()) {
        m_storage.servers.insert(m_storage.servers.begin(), { "Main Server", mainServerUrl });
    } else if (it != m_storage.servers.begin()) {
        std::iter_swap(m_storage.servers.begin(), it);
    }

    if (m_storage.activeIdx >= m_storage.servers.size()) {
        m_storage.activeIdx = 0;
    }
}

}
