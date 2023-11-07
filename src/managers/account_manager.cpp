#include "account_manager.hpp"

#include <util/crypto.hpp>

GLOBED_SINGLETON_DEF(GlobedAccountManager)
GlobedAccountManager::GlobedAccountManager() : box(SecretBox::withPassword("")) {}

void GlobedAccountManager::setSecretKey(const std::string& key) {
    box.setPassword(key);
}

std::string GlobedAccountManager::generateAuthCode() {
    auto b64Token = geode::Mod::get()->getSavedValue<std::string>("auth-totp-key");
    if (b64Token.empty()) {
        throw std::runtime_error("unable to generate auth code: no token");
    }

    auto encToken = util::crypto::base64Decode(b64Token);
    
    auto decToken = box.decrypt(encToken);

    return util::crypto::simpleTOTP(decToken);
}

void GlobedAccountManager::storeAuthKey(const util::data::byte* source, size_t size) {
    auto encrypted = box.encrypt(source, size);
    auto encoded = util::crypto::base64Encode(encrypted);

    geode::Mod::get()->setSavedValue("auth-totp-key", encoded);
}

void GlobedAccountManager::storeAuthKey(const util::data::bytevector& source) {
    storeAuthKey(source.data(), source.size());
}
