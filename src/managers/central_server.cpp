#include "central_server.hpp"

#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <managers/account.hpp>
#include <net/manager.hpp>

CentralServerManager::CentralServerManager() {
    this->reload();

    // if empty (as it would be by default), add our server

    bool empty = _servers.lock()->empty();

    if (empty) {
        this->addServer(CentralServer {
            .name = "Main server",
            .url = "http://globed.dankmeme.dev"
        });
    }

    // if we have a stored active server, use it. otherwise use idx 0

    _activeIdx = 0;

    auto storedActive = Mod::get()->getSavedValue<std::string>(ACTIVE_SERVER_KEY);
    if (!storedActive.empty()) {
        int idx = std::stoi(storedActive);

        if (idx > 0 && idx < _servers.lock()->size()) {
            this->setActive(idx);
        } else if (idx == STANDALONE_IDX) {
            auto& gsm = GameServerManager::get();

            auto addr = gsm.loadStandalone();
            if (!addr.empty()) {
                gsm.clear();

                if (gsm.addServer(GameServerManager::STANDALONE_ID, "Server", addr, "unknown").isOk()) {
                    this->setStandalone();
                } else {
                    _activeIdx = 0;
                }

                gsm.pendingChanges = true;
            }
        }
    }
}

void CentralServerManager::setActive(int index) {
    _activeIdx = index;
    Mod::get()->setSavedValue(ACTIVE_SERVER_KEY, std::to_string(index));
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

    bool doSwitchRoutine = false;

    // if we are removing the current active server, perform the switch routine
    if (index == active) {
        // if this is the last server in the list decrement by 1, otherwise keep the id
        if (index == servers->size() - 1) {
            _activeIdx = active - 1;
        }

        doSwitchRoutine = true;
    } else if (active >= 0 && active < servers->size() && active >= index) {
        _activeIdx = active - 1;
    }

    servers->erase(servers->begin() + index);
    servers.unlock();

    if (doSwitchRoutine) {
        this->switchRoutine(_activeIdx, true);
    }
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

void CentralServerManager::switchRoutine(int index, bool force) {
    if (!force && this->getActiveIndex() == index) return;

    this->setActive(index);
    recentlySwitched = true;

    // clear the authtoken
    auto& gam = GlobedAccountManager::get();
    gam.authToken.lock()->clear();

    // clear game servers
    auto& gsm = GameServerManager::get();
    gsm.clear();
    gsm.pendingChanges = true;

    // disconnect from the server if any
    auto& nm = NetworkManager::get();
    nm.disconnect(false);

    // auto reinitialize account manager
    gam.autoInitialize();
}

void CentralServerManager::reload() {
    auto servers = _servers.lock();
    servers->clear();

    try {
        auto b64value = Mod::get()->getSavedValue<std::string>(SETTING_KEY);
        if (b64value.empty()) return;

        auto decoded = util::crypto::base64Decode(b64value);
        if (decoded.empty()) return;

        ByteBuffer buf(std::move(decoded));
        auto result = buf.readValue<std::vector<CentralServer>>();
        if (result.isErr()) {
            ErrorQueues::get().warn(fmt::format("failed to load servers: {}", result.unwrapErr()));
            return;
        }
        *servers = result.unwrap();
    } catch (const std::exception& e) {
        ErrorQueues::get().warn(std::string("failed to load servers: ") + e.what());
    }
}

// lmao at this point i decided it's easier to just binary encode it than mess with base64 string concatenation
void CentralServerManager::save() {
    auto servers = _servers.lock();

    ByteBuffer buf;
    buf.writeValue<std::vector<CentralServer>>(*servers);
    auto data = util::crypto::base64Encode(buf.data());

    Mod::get()->setSavedValue(SETTING_KEY, data);

    Mod::get()->setSavedValue(ACTIVE_SERVER_KEY, std::to_string(_activeIdx.load()));
}

