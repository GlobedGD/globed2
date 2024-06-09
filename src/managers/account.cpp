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

std::string GlobedAccountManager::getAuthKey() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::getAuthKey before initializing the instance")

    auto jsonkey = this->getKeyFor("auth-totp-key");
    return Mod::get()->getSavedValue<std::string>(jsonkey);
}

void GlobedAccountManager::requestAuthToken(
    const std::string_view baseUrl,
    std::optional<std::function<void()>> callback
) {
    auto authkey = this->getAuthKey();
    // recode as urlsafe
    authkey = util::crypto::base64Encode(util::crypto::base64Decode(authkey), util::crypto::Base64Variant::URLSAFE);

    auto gd = gdData.lock();

    this->cancelAuthTokenRequest();

    requestCallbackStored = std::move(callback);
    log::debug("base url: {}", baseUrl);

    auto request = web::WebRequest()
        .userAgent(util::net::webUserAgent())
        .timeout(util::time::seconds(3))
        .param("aid", gd->accountId)
        .param("uid", gd->userId)
        .param("aname", gd->accountName)
        .param("authkey", authkey)
        .post(fmt::format("{}/totplogin", baseUrl))
        .map([this](web::WebResponse* response) -> Result<std::string, std::string> {
            GLOBED_UNWRAP_INTO(response->string(), auto resptext);

            if (resptext.empty()) {
                return Err(fmt::format("server sent an empty response with code {}", response->code()));
            }

            if (response->ok()) {
                return Ok(resptext);
            } else {
                return Err(resptext);
            }

        }, [](web::WebProgress* progress) -> std::monostate {
            return {};
        });

    requestListener.bind(this, &GlobedAccountManager::requestCallback);
    requestListener.setFilter(std::move(request));
}

void GlobedAccountManager::requestCallback(typename Task<Result<std::string, std::string>>::Event* event) {
    if (!event || !event->getValue()) return;

    if (event->getValue()->isOk()) {
        *this->authToken.lock() = event->getValue()->unwrap();

        if (requestCallbackStored.has_value()) {
            requestCallbackStored.value()();
        }
        return;
    }

    std::string message = event->getValue()->unwrapErr();

    ErrorQueues::get().error(fmt::format(
        "Failed to generate a session token! Please try to login and connect again.\n\nReason: <cy>{}</c>",
        message
    ));

    this->clearAuthKey();
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