#include "account.hpp"

#include <defs/geode.hpp>

#include <managers/central_server.hpp>
#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <util/crypto.hpp>
#include <util/format.hpp>
#include <util/misc.hpp>
#include <util/net.hpp>

using namespace geode::prelude;

GlobedAccountManager::GlobedAccountManager() {
    // TODO 1.6.x: remove the migration.
    this->migrateOldValues();
}

void GlobedAccountManager::migrateOldValues() {
    auto& container = Mod::get()->getSaveContainer().as_object();

    std::vector<std::pair<std::string, std::string>> toPush;
    std::vector<std::string> toErase;

    for (auto& [key, value] : container) {
        std::string pfx;
        if (key.starts_with("stored-admin-pw-")) {
            pfx = "stored-admin-pw-";
        } else if (key.starts_with("auth-totp-key-")) {
            pfx = "auth-totp-key-";
        }

        if (pfx.empty()) continue;

        if (value.as_string().empty()) {
            toErase.push_back(key);
            continue;
        }

        // log::debug("key: {}", key);

        try {
            auto [_k, _sep, hash] = util::format::partition(key, pfx);

            auto cryptoKey = util::crypto::hexDecode(hash);

            SecretBox box(cryptoKey);
            auto adminPwd = value.as_string();

            util::data::bytevector password;
            if (pfx == "stored-admin-pw-") {
                password = box.decrypt(util::crypto::base64Decode(adminPwd));
            } else {
                password = util::crypto::base64Decode(adminPwd);
            }

            // log::debug("raw password: {}", password);

            // append the per-device fingerprint
            auto fp = util::misc::fingerprint().getRaw();
            cryptoKey.insert(cryptoKey.end(), fp.begin(), fp.end());

            SecretBox box2(util::crypto::simpleHash(cryptoKey));
            auto encrypted = box2.encrypt(password);

            // replace - with 2- at the end
            std::string newpfx(pfx);
            newpfx.pop_back();
            newpfx.push_back('2');
            newpfx.push_back('-');

            toPush.push_back({util::format::replace(key, pfx, newpfx), util::crypto::base64Encode(encrypted)});

        } catch (const std::exception& e) {
            log::warn("Failed to migrate admin password: {}", e.what());
        }

        toErase.push_back(key);
    }

    for (const auto& key : toErase) {
        container.erase(key);
    }

    for (const auto& [key, value] : toPush) {
        // log::debug("migrate {} value {}", key, value);
        container[key] = value;
    }
}

void GlobedAccountManager::initialize(const std::string_view name, int accountId, int userId, const std::string_view central) {
    GDData data = {
        .accountName = std::string(name),
        .accountId = accountId,
        .userId = userId,
        .central = std::string(central),
        .precomputedHash = this->computeGDDataHash(name, accountId, userId, central)
    };

    *gdData.lock() = data;

    auto cryptoKey = util::crypto::hexDecode(data.precomputedHash);

    // append the per-device fingerprint
    auto fp = util::misc::fingerprint().getRaw();
    cryptoKey.insert(cryptoKey.end(), fp.begin(), fp.end());

    cryptoBox = std::make_unique<SecretBox>(util::crypto::simpleHash(cryptoKey));

    initialized = true;
}

void GlobedAccountManager::autoInitialize() {
    auto* gjam = GJAccountManager::sharedState();
    auto& csm = CentralServerManager::get();

    std::string activeCentralUrl = "";

    auto activeCentral = csm.getActive();
    if (activeCentral) {
        activeCentralUrl = activeCentral.value().url;
    }

    this->initialize(gjam->m_username, gjam->m_accountID, GameManager::get()->m_playerUserID.value(), activeCentralUrl);
}

void GlobedAccountManager::storeAuthKey(const util::data::byte* source, size_t size) {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::storeAuthKey before initializing the instance")

    auto encrypted = cryptoBox->encrypt(source, size);
    auto encoded = util::crypto::base64Encode(encrypted);

    Mod::get()->setSavedValue(this->getKeyFor("auth-totp-key2"), encoded);
}

void GlobedAccountManager::storeAuthKey(const util::data::bytevector& source) {
    storeAuthKey(source.data(), source.size());
}

void GlobedAccountManager::clearAuthKey() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::clearAuthKey before initializing the instance")

    Mod::get()->setSavedValue<std::string>(this->getKeyFor("auth-totp-key2"), "");
}

bool GlobedAccountManager::hasAuthKey() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::hasAuthKey before initializing the instance")

    auto jsonkey = this->getKeyFor("auth-totp-key2");
    auto b64Token = Mod::get()->getSavedValue<std::string>(jsonkey);

    if (b64Token.empty()) {
        return false;
    }

    try {
        cryptoBox->decrypt(util::crypto::base64Decode(b64Token));
        return true;
    } catch (const std::exception& e) {}

    return false;
}

std::optional<std::string> GlobedAccountManager::getAuthKey() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::getAuthKey before initializing the instance")

    auto jsonkey = this->getKeyFor("auth-totp-key2");
    auto encoded = Mod::get()->getSavedValue<std::string>(jsonkey);

    if (encoded.empty()) {
        return std::nullopt;
    }

    try {
        auto decrypted = cryptoBox->decrypt(util::crypto::base64Decode(encoded));
        return util::crypto::base64Encode(decrypted, util::crypto::Base64Variant::URLSAFE);
    } catch (const std::exception& e) {
        log::warn("Failed to load authkey: {}", e.what());
        return std::nullopt;
    }
}

void GlobedAccountManager::requestAuthToken(std::optional<std::function<void()>> callback) {
    this->cancelAuthTokenRequest();

    requestCallbackStored = std::move(callback);

    auto task = WebRequestManager::get().requestAuthToken();

    requestListener.bind(this, &GlobedAccountManager::requestCallback);
    requestListener.setFilter(task);
}

void GlobedAccountManager::requestCallback(WebRequestManager::Task::Event* event) {
    if (!event || !event->getValue()) return;

    auto result = std::move(*event->getValue());

    if (result.ok()) {
        *this->authToken.lock() = std::move(result.text().unwrapOrDefault());

        if (requestCallbackStored.has_value()) {
            requestCallbackStored.value()();
        }

        return;
    }

    auto rawError = result.text().unwrapOrDefault();

    std::string reason;
    if (result.getCode() == 401) {
        // invalid auth? or banned
        if (rawError.empty()) {
            reason = "unauthorized, please reauthenticate";
        } else {
            reason = fmt::format("unauthorized: {}", rawError);
        }
    }

    if (reason.empty()) {
        reason = result.getError();
    }

    ErrorQueues::get().error(fmt::format(
        "Failed to generate a session token! Please try to login and connect again.\n\nReason: <cy>{}</c>",
        reason
    ));

    this->clearAuthKey();
}

void GlobedAccountManager::storeAdminPassword(const std::string_view password) {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::storeAdminPassword before initializing the instance")

    auto jsonkey = this->getKeyFor("stored-admin-pw2");

    auto encrypted = cryptoBox->encrypt(password);
    auto encoded = util::crypto::base64Encode(encrypted);

    Mod::get()->setSavedValue(jsonkey, encoded);
}

void GlobedAccountManager::storeTempAdminPassword(std::string_view password) {
    *tempAdminPassword.lock() = std::string(password);
}

std::string GlobedAccountManager::getTempAdminPassword() {
    return *tempAdminPassword.lock();
}

void GlobedAccountManager::clearAdminPassword() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::clearAdminPassword before initializing the instance")

    auto jsonkey = this->getKeyFor("stored-admin-pw2");
    Mod::get()->setSavedValue<std::string>(jsonkey, "");
}

bool GlobedAccountManager::hasAdminPassword() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::getAdminPassword before initializing the instance")

    auto jsonkey = this->getKeyFor("stored-admin-pw2");
    return !Mod::get()->getSavedValue<std::string>(jsonkey).empty();
}

std::optional<std::string> GlobedAccountManager::getAdminPassword() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::getAdminPassword before initializing the instance")

    auto jsonkey = this->getKeyFor("stored-admin-pw2");
    auto b64pw = Mod::get()->getSavedValue<std::string>(jsonkey);
    try {
        auto data = util::crypto::base64Decode(b64pw);
        auto adminPassword = cryptoBox->decryptToString(data);
        return adminPassword;
    } catch (const std::exception& e) {
        log::warn("failed to read admin key: {}", e.what());
        return std::nullopt;
    }
}

void GlobedAccountManager::cancelAuthTokenRequest() {
    requestListener.getFilter().cancel();
}

std::string GlobedAccountManager::computeGDDataHash(const std::string_view name, int accountId, int userId, const std::string_view central) {
    auto hash = util::crypto::simpleHash(fmt::format(
        "{}-{}-{}-{}", name, accountId, userId, central
    ));

    return util::crypto::hexEncode(hash);
}

// NOTE: this does not check for initialized, callers must do it themselves
std::string GlobedAccountManager::getKeyFor(const std::string_view key) {
    return std::string(key) + "-" + gdData.lock()->precomputedHash;
}