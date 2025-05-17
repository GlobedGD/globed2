#include "account.hpp"

#include <defs/geode.hpp>
#include <managers/central_server.hpp>
#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <managers/friend_list.hpp>
#include <util/crypto.hpp>
#include <util/format.hpp>
#include <util/misc.hpp>
#include <util/net.hpp>

#include <argon/argon.hpp>

using namespace geode::prelude;
using namespace util::data;

GlobedAccountManager::GlobedAccountManager() {}

void GlobedAccountManager::initialize(std::string_view name, int accountId, int userId, std::string_view central) {
    bool accountChanged = gdData.lock()->accountId != accountId;

    GDData data = {
        .accountName = std::string(name),
        .accountId = accountId,
        .userId = userId,
        .central = std::string(central),
        .precomputedHash = this->computeGDDataHash(name, accountId, userId, central)
    };

    *gdData.lock() = data;

    auto cryptoKey = std::move(util::crypto::hexDecode(data.precomputedHash).unwrap());

    // append the per-device fingerprint
    auto fp = util::misc::fingerprint().getRaw();
    cryptoKey.insert(cryptoKey.end(), fp.begin(), fp.end());

    cryptoBox = std::make_unique<SecretBox>(util::crypto::simpleHash(cryptoKey));

    initialized = true;

    if (accountChanged) {
        // if we changed accounts, clear some stuff
        this->accountWasChanged();
        argonToken.lock()->clear();
    }
}

void GlobedAccountManager::autoInitialize() {
    auto* gjam = GJAccountManager::sharedState();
    auto& csm = CentralServerManager::get();

    std::string activeCentralUrl = "";

    auto activeCentral = csm.getActive();
    if (activeCentral) {
        activeCentralUrl = activeCentral.value().url;
    }

    this->initialize(gjam->m_username, gjam->m_accountID, globed::cachedSingleton<GameManager>()->m_playerUserID.value(), activeCentralUrl);
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

    auto res = util::crypto::base64Decode(b64Token);
    if (!res) return false;

    return cryptoBox->decrypt(res.unwrap()).isOk();
}

std::optional<std::string> GlobedAccountManager::getAuthKey() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::getAuthKey before initializing the instance")

    auto jsonkey = this->getKeyFor("auth-totp-key2");
    auto encoded = Mod::get()->getSavedValue<std::string>(jsonkey);

    if (encoded.empty()) {
        return std::nullopt;
    }

    auto res1 = util::crypto::base64Decode(encoded);
    if (!res1) {
        ErrorQueues::get().debugWarn(fmt::format("Failed to load authkey: {}", res1.unwrapErr()));
        return std::nullopt;
    }

    auto res = cryptoBox->decrypt(res1.unwrap());
    if (!res) {
        ErrorQueues::get().debugWarn(fmt::format("Failed to load authkey: {}", res.unwrapErr()));
        return std::nullopt;
    }

    auto decrypted = std::move(res.unwrap());

    return util::crypto::base64Encode(decrypted, util::crypto::Base64Variant::URLSAFE);
}

void GlobedAccountManager::storeArgonToken(const std::string& token) {
    *argonToken.lock() = token;
}

std::string GlobedAccountManager::getArgonToken() {
    return *argonToken.lock();
}

void GlobedAccountManager::requestAuthToken(std::optional<std::function<void(bool)>> callback, bool argon) {
    this->cancelAuthTokenRequest();

    requestCallbackStored = std::move(callback);
    didUseArgon = argon;

    auto task = WebRequestManager::get().requestAuthToken(argon);

    requestListener.bind(this, &GlobedAccountManager::requestCallback);
    requestListener.setFilter(task);
}

void GlobedAccountManager::requestCallback(WebRequestManager::Task::Event* event) {
    if (!event || !event->getValue()) return;

    auto result = std::move(*event->getValue());

    if (result.ok()) {
        *this->authToken.lock() = std::move(result.text().unwrapOrDefault());

        if (requestCallbackStored.has_value()) {
            requestCallbackStored.value()(true);
        }

        requestCallbackStored.reset();

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

    if (didUseArgon) {
        argon::clearToken();
    } else {
        this->clearAuthKey();
    }


    if (requestCallbackStored.has_value()) {
        requestCallbackStored.value()(false);
    }

    requestCallbackStored.reset();
}

void GlobedAccountManager::storeAdminPassword(std::string_view password) {
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

    auto res1 = util::crypto::base64Decode(b64pw);
    if (!res1) {
        ErrorQueues::get().debugWarn(fmt::format("Failed to read admin password: {}", res1.unwrapErr()));
        this->storeAdminPassword("");
        return std::nullopt;
    }

    auto res2 = cryptoBox->decryptToString(res1.unwrap());
    if (!res2) {
        ErrorQueues::get().debugWarn(fmt::format("Failed to read admin password: {}", res2.unwrapErr()));
        this->storeAdminPassword("");
        return std::nullopt;
    }

    return res2.unwrap();
}

void GlobedAccountManager::cancelAuthTokenRequest() {
    requestListener.getFilter().cancel();
}

void GlobedAccountManager::accountWasChanged() {
    FriendListManager::get().invalidate();
}

std::string GlobedAccountManager::computeGDDataHash(std::string_view name, int accountId, int userId, std::string_view central) {
    auto hash = util::crypto::simpleHash(fmt::format(
        "{}-{}-{}-{}", name, accountId, userId, central
    ));

    return util::crypto::hexEncode(hash);
}

// NOTE: this does not check for initialized, callers must do it themselves
std::string GlobedAccountManager::getKeyFor(std::string_view key) {
    return std::string(key) + "-" + gdData.lock()->precomputedHash;
}
