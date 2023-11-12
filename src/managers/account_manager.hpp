#pragma once

#include <defs.hpp>
#include <crypto/secret_box.hpp>
#include <util/sync.hpp>

class GlobedAccountManager {
public:
    GLOBED_SINGLETON(GlobedAccountManager)

    int32_t accountId;
    std::string accountName;
    util::sync::WrappingMutex<std::string> authToken;
    
    GlobedAccountManager();

    // usually user's GJP.
    void setSecretKey(const std::string& key);

    void storeAuthKey(const util::data::byte* source, size_t size);
    void storeAuthKey(const util::data::bytevector& source);

    std::string generateAuthCode();

private:
    SecretBox box;
};