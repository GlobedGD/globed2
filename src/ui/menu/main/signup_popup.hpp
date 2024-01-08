#pragma once
#include <defs.hpp>

class GlobedSignupPopup : public geode::Popup<>, public CommentUploadDelegate {
public:
    constexpr static float POPUP_WIDTH = 180.f;
    constexpr static float POPUP_HEIGHT = 80.f;

    static GlobedSignupPopup* create();

protected:
    cocos2d::CCLabelBMFont* statusMessage;

    std::string storedAuthcode;
    int storedLevelId;

    bool setup() override;
    void keyDown(cocos2d::enumKeyCodes key) override;
    void keyBackClicked() override;
    void onFailure(const std::string_view message);
    void onSuccess();

    void onChallengeCreated(int levelId, const std::string_view chtoken);
    void onChallengeCompleted(const std::string_view authcode);

    void commentUploadFinished(int) override;
    void commentUploadFailed(int, CommentError) override;
    void commentDeleteFailed(int, int) override;

    void onDelayedChallengeCompleted();
};