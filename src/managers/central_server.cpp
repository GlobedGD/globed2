#include "central_server.hpp"

#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <managers/account.hpp>
#include <net/manager.hpp>

#include <matjson/reflect.hpp>
#include <matjson/std.hpp>
#include <matjson/stl_serialize.hpp>

#include <asp/fs.hpp>

static std::string identForServer(const CentralServer& server) {
    auto s = util::crypto::hexEncode(util::crypto::simpleHash(server.url));
    s.resize(16);

    return s;
}

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

    return servers->at(idx).server;
}

int CentralServerManager::getActiveIndex() {
    return _activeIdx.load();
}

std::vector<CentralServer> CentralServerManager::getAllServers() {
    std::vector<CentralServer> out;
    for (auto& server : *_servers.lock()) {
        out.push_back(server.server);
    }

    return out;
}

CentralServer CentralServerManager::getServer(int index) {
    return _servers.lock()->at(index).server;
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
    return _servers.lock()->at(0).server.url == globed::string<"main-server-url">();
}

void CentralServerManager::addServer(const CentralServer& data) {
    _servers.lock()->push_back(CentralServerData {
        .server = data,
        .fetchedMeta = false
    });
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

    servers->at(index).server = data;
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

void CentralServerManager::updateCacheForActive(const std::string& response) {
    auto server = this->getActive();
    if (!server) return;

    auto ident = identForServer(*server);

    auto key = fmt::format("_server-cache-{}", ident);
    Mod::get()->setSavedValue(key, response);
}

Result<> CentralServerManager::initFromCache() {
    auto server = this->getActive();
    if (!server) return Err("no active server");

    auto cachedResp = Mod::get()->getSavedValue<std::string>(fmt::format("_server-cache-{}", identForServer(*server)));
    if (cachedResp.empty()) {
        return Err("no cache");
    }

    auto resp = GEODE_UNWRAP(matjson::parseAs<MetaResponse>(cachedResp));
    this->initFromMeta(resp);

    return Ok();
}

void CentralServerManager::initFromMeta(const MetaResponse& resp) {
    auto server = this->getServerInternal(_activeIdx);
    if (!server) return;

    auto& data = *server.value();

    data.fetchedMeta = true;
    data.auth = resp.auth;
    data.argonUrl = resp.argon;

    auto& gsm = GameServerManager::get();

    for (auto& server : resp.servers) {
        auto result = gsm.addOrUpdateServer(
            server.id,
            server.name,
            server.address,
            server.region
        );

        if (result.unwrapOr(false)) {
            gsm.pendingChanges = true;
        }
    }

    for (auto& relay : resp.relays.value_or(std::vector<ServerRelay>{})) {
        gsm.addOrUpdateRelay(relay);
    }

    gsm.reloadActiveRelay();
}

bool CentralServerManager::activeHasAuth() {
    auto server = this->getServerInternal(_activeIdx);
    if (!server) return false;

    auto& data = *server.value();
    return data.auth;
}

std::optional<std::string> CentralServerManager::activeArgonUrl() {
    auto server = this->getServerInternal(_activeIdx);
    if (!server) return std::nullopt;

    auto& data = *server.value();
    return data.argonUrl;
}

std::optional<CentralServerManager::CentralServerData*> CentralServerManager::getServerInternal(int idx) {
    auto servers = _servers.lock();
    if (idx < 0 || idx >= servers->size()) {
        return std::nullopt;
    }

    return &(*servers)[idx];
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
        servers->push_back(CentralServerData {
            .server = server,
            .fetchedMeta = false
        });
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

    servers->push_back(CentralServerData {
        .server = CentralServer {
            .name = "Main server",
            .url = url,
        }
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
        buf.writeValue((*servers)[i].server);
    }

    auto data = util::crypto::base64Encode(buf.data());

    Mod::get()->setSavedValue(SETTING_KEY, data);

    Mod::get()->setSavedValue(ACTIVE_SERVER_KEY, std::to_string(_activeIdx.load()));

    (void) Mod::get()->saveData();
}

