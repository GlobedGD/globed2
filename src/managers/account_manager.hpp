#pragma once

#include <defs.hpp>
#include <crypto/secret_box.hpp>

class GlobedAccountManager {
public:
    GLOBED_SINGLETON(GlobedAccountManager)
    GlobedAccountManager();

    // usually user's GJP.
    void setSecretKey(const std::string& key);

    void storeAuthKey(const util::data::byte* source, size_t size);
    void storeAuthKey(const util::data::bytevector& source);

    std::string generateAuthCode();

private:
    SecretBox box;
};