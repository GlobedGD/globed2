#pragma once
#include <defs/all.hpp>

class GlobedSignupPopup : public geode::Popup<>, public UploadMessageDelegate {
public:
    constexpr static float POPUP_WIDTH = 180.f;
    constexpr static float POPUP_HEIGHT = 80.f;

    static GlobedSignupPopup* create();

protected:
    cocos2d::CCLabelBMFont* statusMessage;

    std::string storedAuthcode;
    int storedAccountId;

    bool setup() override;
    void keyDown(cocos2d::enumKeyCodes key) override;
    void keyBackClicked() override;
    void onFailure(const std::string_view message);
    void onSuccess();

    void onChallengeCreated(int accountId, const std::string_view chtoken);
    void onChallengeCompleted(const std::string_view authcode);

    void uploadMessageFinished(int) override;
    void uploadMessageFailed(int) override;

    void onDelayedChallengeCompleted();
};