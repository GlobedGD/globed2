#pragma once

#include <defs.hpp>
#include <crypto/secret_box.hpp>
#include <util/sync.hpp>

// This class is not guaranteed to be fully thread safe.
class GlobedAccountManager {
    GLOBED_SINGLETON(GlobedAccountManager)

    int32_t accountId;
    std::string accountName;
    util::sync::WrappingMutex<std::string> authToken;
    
    GlobedAccountManager();

    // usually user's GJP.
    void setSecretKey(const std::string& key);

    void storeAuthKey(const util::data::byte* source, size_t size);
    void storeAuthKey(const util::data::bytevector& source);

    bool hasAuthKey();

    std::string generateAuthCode();

private:
    SecretBox box;
};