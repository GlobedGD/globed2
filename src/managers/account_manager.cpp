#include "account_manager.hpp"

#include <util/crypto.hpp>

GLOBED_SINGLETON_DEF(GlobedAccountManager)
GlobedAccountManager::GlobedAccountManager() : box(SecretBox::withPassword("")) {}

void GlobedAccountManager::initialize(const std::string& name, int accountId, const std::string& gjp, const std::string& central) {
    GDData data = {
        .accountName = name,
        .accountId = accountId,
        .gjp = gjp,
        .central = central,
        .precomputedHash = this->computeGDDataHash(name, accountId, gjp, central)
    };

    box.setPassword(gjp);

    *gdData.lock() = data;
    initialized = true;
}

std::string GlobedAccountManager::generateAuthCode() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::generateAuthCode before initializing the instance")

    auto jsonkey = this->getKeyFor("auth-totp-key");
    auto b64Token = geode::Mod::get()->getSavedValue<std::string>(jsonkey);

    GLOBED_REQUIRE(!b64Token.empty(), "unable to generate auth code: no token")

    auto encToken = util::crypto::base64Decode(b64Token);
    auto decToken = box.decrypt(encToken);

    return util::crypto::simpleTOTP(decToken);
}

void GlobedAccountManager::storeAuthKey(const util::data::byte* source, size_t size) {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::storeAuthKey before initializing the instance")

    auto encrypted = box.encrypt(source, size);
    auto encoded = util::crypto::base64Encode(encrypted);

    geode::Mod::get()->setSavedValue(this->getKeyFor("auth-totp-key"), encoded);
}

void GlobedAccountManager::storeAuthKey(const util::data::bytevector& source) {
    storeAuthKey(source.data(), source.size());
}

void GlobedAccountManager::clearAuthKey() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::clearAuthKey before initializing the instance")

    geode::Mod::get()->setSavedValue<std::string>(this->getKeyFor("auth-totp-key"), "");
}

bool GlobedAccountManager::hasAuthKey() {
    GLOBED_REQUIRE(initialized, "Attempting to call GlobedAccountManager::hasAuthKey before initializing the instance")

    auto jsonkey = this->getKeyFor("auth-totp-key");
    auto b64Token = geode::Mod::get()->getSavedValue<std::string>(jsonkey);
    return !b64Token.empty();
}

std::string GlobedAccountManager::computeGDDataHash(const std::string& name, int accountId, const std::string& gjp, const std::string& central) {
    auto hash = util::crypto::simpleHash(fmt::format(
        "{}-{}-{}-{}", name, accountId, gjp, central
    ));

    return util::crypto::hexEncode(hash);
}

// NOTE: this does not check for initialized, callers must do it themselves
std::string GlobedAccountManager::getKeyFor(const std::string& key) {
    return key + "-" + gdData.lock()->precomputedHash;
}