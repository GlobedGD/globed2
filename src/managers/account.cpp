#include "account.hpp"

#include <defs/geode.hpp>

#include <managers/central_server.hpp>
#include <managers/error_queues.hpp>
#include <managers/game_server.hpp>
#include <util/crypto.hpp>
#include <util/format.hpp>
#include <util/net.hpp>

using namespace geode::prelude;

GlobedAccountManager::GlobedAccountManager() {}

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
    cryptoBox = std::make_unique<SecretBox>(cryptoKey);

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

Result<std::string> GlobedAccountManager::generateAuthCode() {
    GLOBED_REQUIRE_SAFE(initialized, "Attempting to call GlobedAccountManager::generateAuthCode before initializing the instance")

    auto jsonkey = this->getKeyFor("auth-totp-key");
    auto b64Token = Mod::get()->getSavedValue<std::string>(jsonkey);

    GLOBED_REQUIRE_SAFE(!b64Token.empty(), "unable to generate auth code: no token")

    util::data::bytevector decToken;
    try {
        decToken = util::crypto::base64Decode(b64Token);
    } catch (const std::exception& e) {
        return Err(e.what());
    }

    return Ok(util::crypto::simpleTOTP(decToken));
}

void GlobedAccountManager::storeAuthKey(const util::data::byte* source, size_t size) {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::storeAuthKey before initializing the instance")

    auto encoded = util::crypto::base64Encode(source, size);

    Mod::get()->setSavedValue(this->getKeyFor("auth-totp-key"), encoded);
}

void GlobedAccountManager::storeAuthKey(const util::data::bytevector& source) {
    storeAuthKey(source.data(), source.size());
}

void GlobedAccountManager::clearAuthKey() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::clearAuthKey before initializing the instance")

    Mod::get()->setSavedValue<std::string>(this->getKeyFor("auth-totp-key"), "");
}

bool GlobedAccountManager::hasAuthKey() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::hasAuthKey before initializing the instance")

    auto jsonkey = this->getKeyFor("auth-totp-key");
    auto b64Token = Mod::get()->getSavedValue<std::string>(jsonkey);
    return !b64Token.empty();
}

void GlobedAccountManager::requestAuthToken(
    const std::string_view baseUrl,
    int accountId,
    int userId,
    const std::string_view accountName,
    const std::string_view authcode,
    std::optional<std::function<void()>> callback
) {
    auto url = fmt::format(
        "{}/totplogin?aid={}&uid={}&aname={}&code={}",
        baseUrl,
        accountId,
        userId,
        util::format::urlEncode(accountName),
        authcode
    );

    this->cancelAuthTokenRequest();

    requestHandle = web::AsyncWebRequest()
        .userAgent(util::net::webUserAgent())
        .timeout(util::time::seconds(3))
        .post(url)
        .text()
        .then([this, callback](std::string& response) {
            requestHandle = std::nullopt;
            *this->authToken.lock() = response;

            if (callback.has_value()) {
                callback.value()();
            }
        })
        .expect([this](std::string error) {
            requestHandle = std::nullopt;
            ErrorQueues::get().error(fmt::format(
                "Failed to generate a session token! Please try to login and connect again.\n\nReason: <cy>{}</c>",
                error
            ));
            this->clearAuthKey();
        })
        .cancelled([this](auto) {
            requestHandle = std::nullopt;
        })
        .send();
}

void GlobedAccountManager::storeAdminPassword(const std::string_view password) {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::storeAdminPassword before initializing the instance")

    auto jsonkey = this->getKeyFor("stored-admin-pw");

    auto encrypted = cryptoBox->encrypt(password);
    auto encoded = util::crypto::base64Encode(encrypted);

    Mod::get()->setSavedValue(jsonkey, encoded);
}

void GlobedAccountManager::clearAdminPassword() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::clearAdminPassword before initializing the instance")

    auto jsonkey = this->getKeyFor("stored-admin-pw");
    Mod::get()->setSavedValue<std::string>(jsonkey, "");
}

bool GlobedAccountManager::hasAdminPassword() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::getAdminPassword before initializing the instance")

    auto jsonkey = this->getKeyFor("stored-admin-pw");
    return !Mod::get()->getSavedValue<std::string>(jsonkey).empty();
}

std::optional<std::string> GlobedAccountManager::getAdminPassword() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::getAdminPassword before initializing the instance")

    auto jsonkey = this->getKeyFor("stored-admin-pw");
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
    if (requestHandle.has_value()) {
        requestHandle->get()->cancel();
        requestHandle = std::nullopt;
    }
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