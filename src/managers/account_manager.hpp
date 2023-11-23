#pragma once

#include <defs.hpp>
#include <crypto/secret_box.hpp>
#include <util/sync.hpp>

// all methods of GlobedAccountManager will store/load values with keys that are
// user-specific and central-server-specific, so that switching server or accounts doesn't reset authkeys.
// This class is not guaranteed to be fully thread safe (reason: getSavedValue/setSavedValue)
class GlobedAccountManager {
    GLOBED_SINGLETON(GlobedAccountManager)

    struct GDData {
        std::string accountName;
        int accountId;
        std::string gjp;
        std::string central;
        std::string precomputedHash;
    };

    util::sync::AtomicBool initialized = false;
    util::sync::WrappingMutex<GDData> gdData;
    util::sync::WrappingMutex<std::string> authToken;
    
    GlobedAccountManager();

    // This method can be called multiple times, and in fact it is even advised that you do so.
    // It must be called at least once before calling any other method or they will throw an exception.
    void initialize(const std::string& name, int accountId, const std::string& gjp, const std::string& central);

    void storeAuthKey(const util::data::byte* source, size_t size);
    void storeAuthKey(const util::data::bytevector& source);
    void clearAuthKey();

    bool hasAuthKey();

    std::string generateAuthCode();

private:
    SecretBox box;

    std::string computeGDDataHash(const std::string& name, int accountId, const std::string& gjp, const std::string& central);

    // uses the precomputed hash from GDData and prepends it to the given 'key'
    // i.e. getKeyFor("auth-totp-key") => "auth-totp-key-ab12cd34ef"
    std::string getKeyFor(const std::string& key);
};