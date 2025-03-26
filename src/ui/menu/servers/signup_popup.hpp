#pragma once
#include <defs/all.hpp>

#include <managers/web.hpp>

class GlobedSignupPopup : public geode::Popup<>, public UploadMessageDelegate, public MessageListDelegate {
public:
    constexpr static float POPUP_WIDTH = 180.f;
    constexpr static float POPUP_HEIGHT = 80.f;

    static GlobedSignupPopup* create();

protected:
    cocos2d::CCLabelBMFont* statusMessage;
    WebRequestManager::Listener createListener;
    WebRequestManager::Listener finishListener;

    std::string storedAuthcode;
    std::string storedChToken;
    int storedAccountId;
    bool isSecureMode = false;

    bool setup() override;
    void keyDown(cocos2d::enumKeyCodes key) override;
    void keyBackClicked() override;
    void onFailure(std::string_view message);
    void onSuccess();

    void tryCheckMessageCount();

    void onChallengeCreated(int accountId, std::string_view challenge, std::string_view pubkey, bool secureMode);
    void onChallengeCompleted(std::string_view authcode);

    void uploadMessageFinished(int) override;
    void uploadMessageFailed(int) override;

    void loadMessagesFinished(cocos2d::CCArray* p0, char const* p1) override;
    void loadMessagesFailed(char const* p0, GJErrorCode p1) override;
    void forceReloadMessages(bool p0) override;
    void setupPageInfo(gd::string p0, char const* p1) override;

    void onDelayedChallengeCompleted();
    void createCallback(typename WebRequestManager::Event* event);
    void finishCallback(typename WebRequestManager::Event* event);
};