#pragma once
#include <defs/minimal_geode.hpp>

#include <Geode/utils/web.hpp>
#include <asp/sync.hpp>

#include <crypto/secret_box.hpp>
#include <managers/web.hpp>

#include <util/singleton.hpp>

// all methods of GlobedAccountManager will store/load values with keys that are
// user-specific and central-server-specific, so that switching server or accounts doesn't reset authkeys.
// This class is not guaranteed to be fully thread safe (reason: getSavedValue/setSavedValue)
class GlobedAccountManager : public SingletonBase<GlobedAccountManager> {
protected:
    friend class SingletonBase;
    GlobedAccountManager();

public:
    struct GDData {
        std::string accountName;
        int accountId;
        int userId;
        std::string central;
        std::string precomputedHash;
    };

    asp::AtomicBool initialized = false;
    bool didUseArgon = false;
    asp::Mutex<GDData> gdData;
    asp::Mutex<std::string> authToken;

    // This method can be called multiple times, and in fact it is even advised that you do so often.
    // It must be called at least once before calling any other method or they will throw an exception.
    void initialize(std::string_view name, int accountId, int userId, std::string_view central);
    // Grabs the values from other manager classes and calls `initialize` for you.
    void autoInitialize();

    void storeAuthKey(const util::data::byte* source, size_t size);
    void storeAuthKey(const util::data::bytevector& source);
    void clearAuthKey();

    bool hasAuthKey();
    // Returns the authkey in urlsafe base64 encoded form
    std::optional<std::string> getAuthKey();

    void storeArgonToken(const std::string& token);
    std::string getArgonToken();

    void requestAuthToken(std::optional<std::function<void(bool)>> callback, bool argon);

    // admin password stuff

    void storeAdminPassword(std::string_view password);
    void clearAdminPassword();
    bool hasAdminPassword();
    std::optional<std::string> getAdminPassword();

    void storeTempAdminPassword(std::string_view password);
    std::string getTempAdminPassword();

    void accountWasChanged();

private:
    WebRequestManager::Listener requestListener;
    std::optional<std::function<void(bool)>> requestCallbackStored;
    std::unique_ptr<SecretBox> cryptoBox;
    asp::Mutex<std::string> tempAdminPassword;
    asp::Mutex<std::string> argonToken;

    void requestCallback(WebRequestManager::Task::Event* event);
    void cancelAuthTokenRequest();

    std::string computeGDDataHash(std::string_view name, int accountId, int userId, std::string_view central);

    // uses the precomputed hash from GDData and appends it to the given 'key'
    // i.e. getKeyFor("auth-totp-key") => "auth-totp-key-ab12cd34ef"
    std::string getKeyFor(std::string_view key);
};