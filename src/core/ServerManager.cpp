#include <globed/core/ServerManager.hpp>
#include <globed/core/ValueManager.hpp>

#include <asp/fs.hpp>

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
    } else if (m_storage.activeIdx > index) {
        m_storage.activeIdx--;
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

static std::string getMainServerUrl() {
#ifdef GEODE_IS_WINDOWS
    char envbuf[1024];
    size_t count = 0;
    if (0 == getenv_s(&count, envbuf, "GLOBED_MAIN_SERVER_URL") && count > 0) {
        log::debug("Found server override in environment: {}", envbuf);
        return envbuf;
    }
#endif

    auto path1 = geode::dirs::getGameDir();
    auto path2 = Mod::get()->getConfigDir(false);
    auto path3 = Mod::get()->getSaveDir();

    for (const auto& p : {path1, path2, path3}) {
        auto path = p / "globed-server-url.txt";

        if (asp::fs::exists(path)) {
            log::debug("Found server override in: {}", path);
            auto res = asp::fs::readToString(path);
            if (!res) {
                log::warn("Failed to read file: {}", path);
                continue;
            }

            return std::move(res).unwrap();
        }
    }

    return GLOBED_DEFAULT_MAIN_SERVER_URL;
}

void ServerManager::reload() {
    if (auto opt = globed::value<Storage>("core.central-servers")) {
        m_storage = std::move(*opt);
    } else {
        m_storage = {};
    }

    // ensure the main server is always present and is the first one
    auto mainServerUrl = getMainServerUrl();

    auto it = std::find_if(
        m_storage.servers.begin(), m_storage.servers.end(),
        [&](const CentralServerData& server) { return server.url == mainServerUrl; }
    );

    if (it == m_storage.servers.end()) {
        m_storage.servers.insert(m_storage.servers.begin(), { "Globed Server", mainServerUrl });
    } else if (it != m_storage.servers.begin()) {
        std::iter_swap(m_storage.servers.begin(), it);
    }

    if (m_storage.activeIdx >= m_storage.servers.size()) {
        m_storage.activeIdx = 0;
    }
}

bool ServerManager::isOfficialServerActive() {
    return this->getActiveServer().url == GLOBED_DEFAULT_MAIN_SERVER_URL;
}

}
