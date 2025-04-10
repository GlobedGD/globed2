#include "central_server.hpp"

#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <managers/account.hpp>
#include <net/manager.hpp>

#include <asp/fs.hpp>

CentralServerManager::CentralServerManager() {
    this->reload();

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

bool CentralServerManager::isOfficialServerActive() {
    return _servers.lock()->at(0).url == globed::string<"main-server-url">();
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

    this->save();
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

std::string CentralServerManager::getMotdKey() {
    if (_activeIdx.load() == STANDALONE_IDX) {
        return "";
    }

    auto active = this->getActive();
    if (!active) {
        return "";
    }

    return fmt::format("last-seen-motd-{}", active->url);
}

Result<> CentralServerManager::tryReload() {
    auto b64value = Mod::get()->getSavedValue<std::string>(SETTING_KEY);
    if (b64value.empty()) return Ok();

    GLOBED_UNWRAP_INTO(util::crypto::base64Decode(b64value), auto decoded);
    if (decoded.empty()) return Ok();

    ByteBuffer buf(std::move(decoded));
    auto result = buf.readValue<std::vector<CentralServer>>();
    if (result.isErr()) {
        ErrorQueues::get().warn(fmt::format("failed to load servers: {}", result.unwrapErr()));
        return Ok();
    }

    auto parsed = result.unwrap();

    auto servers = _servers.lock();
    for (const auto& server : parsed) {
        servers->push_back(server);
    }

    return Ok();
}

static bool maybeOverrideMainServer(std::string& url) {
#ifdef GEODE_IS_WINDOWS
    // first try env variable
    char buffer[1024];
    size_t count = 0;
    if (0 == getenv_s(&count, buffer, "GLOBED_MAIN_SERVER_URL") && count != 0) {
        log::debug("Found server override in: environment variable");
        url.assign(buffer);
        return true;
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

            url = std::move(res).unwrap();
            return true;
        }
    }

    return false;
}

void CentralServerManager::reload() {
    auto servers = _servers.lock();
    servers->clear();

    // main server is always forced into 1st position

    std::string url = globed::string<"main-server-url">();

    // check for main server overrides.
    if (maybeOverrideMainServer(url)) {
        log::debug("Main server override enabled: {}", url);
    }

    servers->push_back(CentralServer {
        .name = "Main server",
        .url = url,
    });

    servers.unlock();

    auto res = this->tryReload();

    if (!res) {
        ErrorQueues::get().warn(std::string("failed to load servers: ") + res.unwrapErr());
    }
}

// lmao at this point i decided it's easier to just binary encode it than mess with base64 string concatenation
void CentralServerManager::save() {
    auto servers = _servers.lock();

    ByteBuffer buf;

    buf.writeLength(servers->size() - 1);
    for (size_t i = 1; i < servers->size(); i++) {
        buf.writeValue((*servers)[i]);
    }

    auto data = util::crypto::base64Encode(buf.data());

    Mod::get()->setSavedValue(SETTING_KEY, data);

    Mod::get()->setSavedValue(ACTIVE_SERVER_KEY, std::to_string(_activeIdx.load()));

    (void) Mod::get()->saveData();
}

