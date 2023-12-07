#include "central_server_manager.hpp"

#include <managers/error_queues.hpp>

GLOBED_SINGLETON_DEF(CentralServerManager)

CentralServerManager::CentralServerManager() {
    this->reload();

    // if empty (as it would be by default), add our server

    bool empty = _servers.lock()->empty();

    if (empty) {
        this->addServer(CentralServer {
            .name = "Main server",
            .url = "https://globed.dankmeme.dev"
        });
    }

    // if we have a stored active server, use it. otherwise use idx 0

    _activeIdx = 0;

    auto storedActive = geode::Mod::get()->getSavedValue<std::string>(ACTIVE_SERVER_KEY);
    if (!storedActive.empty()) {
        int idx = std::stoi(storedActive);

        if (idx > 0 && idx < _servers.lock()->size()) {
            this->setActive(idx);
        }
    }
}

void CentralServerManager::setActive(int index) {
    _activeIdx = index;
    geode::Mod::get()->setSavedValue(ACTIVE_SERVER_KEY, std::to_string(index));
}

std::optional<CentralServer> CentralServerManager::getActive() {
    int idx = _activeIdx.load();

    if (idx == STANDALONE_IDX) {
        return CentralServer {
            .name = "Standalone server",
            .url = "__standalone__url__sub__",
        };
    }

    auto servers = _servers.lock();

    if (idx < 0 || idx >= servers->size()) {
        return std::nullopt;
    }

    return servers->at(idx);
}

int CentralServerManager::getActiveIndex() {
    return _activeIdx.load();
}

std::vector<CentralServer> CentralServerManager::getAllServers() {
    return *_servers.lock();
}

CentralServer CentralServerManager::getServer(int index) {
    return _servers.lock()->at(index);
}

void CentralServerManager::setStandalone(bool status) {
    this->setActive(status ? STANDALONE_IDX : 0);
}

bool CentralServerManager::standalone() {
    return _activeIdx == STANDALONE_IDX;
}

size_t CentralServerManager::count() {
    return _servers.lock()->size();
}

void CentralServerManager::addServer(const CentralServer& data) {
    _servers.lock()->push_back(data);
    this->save();
}

void CentralServerManager::removeServer(int index) {
    auto servers = _servers.lock();

    if (index < 0 || index >= servers->size()) {
        return;
    }

    // we may want to recalculate the active server index if there's an active server right now

    int active = _activeIdx.load();
    if (active >= 0 && active < servers->size() && active >= index) {
        _activeIdx = active - 1;
    }

    servers->erase(servers->begin() + index);
}

void CentralServerManager::modifyServer(int index, const CentralServer& data) {
    auto servers = _servers.lock();

    if (index < 0 || index >= servers->size()) {
        return;
    }

    servers->at(index) = data;
    servers.unlock();

    this->save();
}

void CentralServerManager::reload() {
    auto servers = _servers.lock();
    servers->clear();

    try {
        auto b64value = geode::Mod::get()->getSavedValue<std::string>(SETTING_KEY);
        if (b64value.empty()) return;

        auto decoded = util::crypto::base64Decode(b64value);
        if (decoded.empty()) return;

        ByteBuffer buf(decoded);
        *servers = buf.readValueVector<CentralServer>();
    } catch (const std::exception& e) {
        ErrorQueues::get().warn(std::string("failed to load servers: ") + e.what());
    }
}

// lmao at this point i decided it's easier to just binary encode it than mess with base64 string concatenation
void CentralServerManager::save() {
    auto servers = _servers.lock();

    ByteBuffer buf;
    buf.writeValueVector(*servers);
    auto data = util::crypto::base64Encode(buf.getDataRef());

    geode::Mod::get()->setSavedValue(SETTING_KEY, data);

    geode::Mod::get()->setSavedValue(ACTIVE_SERVER_KEY, std::to_string(_activeIdx.load()));
}

