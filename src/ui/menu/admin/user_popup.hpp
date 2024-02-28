#pragma once
#include <defs/all.hpp>

#include <Geode/ui/TextInput.hpp>

#include <data/types/admin.hpp>
#include <data/types/gd.hpp>

class AdminUserPopup : public geode::Popup<const UserEntry&, const std::optional<PlayerRoomPreviewAccountData>&>, public UserInfoDelegate {
public:
    static constexpr float POPUP_WIDTH = 300.f;
    static constexpr float POPUP_HEIGHT = 260.f;
    static constexpr int TAG_BAN = 41;
    static constexpr int TAG_MUTE = 42;
    static constexpr int TAG_WHITELIST = 43;

    static AdminUserPopup* create(const UserEntry& userEntry, const std::optional<PlayerRoomPreviewAccountData>& accountData);

private:
    UserEntry userEntry;
    std::optional<PlayerRoomPreviewAccountData> accountData;
    GJUserScore* userScore = nullptr;
    Ref<LoadingCircle> loadingCircle = nullptr;
    ColorChannelSprite* nameColorSprite = nullptr;
    cocos2d::CCLabelBMFont* banDurationText = nullptr;
    geode::TextInput *inputReason = nullptr;
    geode::TextInput *inputAdminPassword = nullptr;
    cocos2d::CCMenu* nameLayout;
    Ref<CCMenuItemSpriteExtra> roleModifyButton;

    bool setup(const UserEntry& userEntry, const std::optional<PlayerRoomPreviewAccountData>& accountData) override;
    void onProfileLoaded();
    void onOpenProfile(cocos2d::CCObject*);
    void onColorSelected(cocos2d::ccColor3B);
    void onViolationChanged(cocos2d::CCObject* sender);
    void onViolationDurationChanged(cocos2d::CCObject* sender);
    void recreateRoleModifyButton();
    cocos2d::ccColor3B getCurrentNameColor();

    void sendUpdateUser();

    void onClose(cocos2d::CCObject*) override;
    void removeLoadingCircle();

    void getUserInfoFinished(GJUserScore* p0) override;
    void getUserInfoFailed(int p0) override;
    void userInfoChanged(GJUserScore* p0) override;
};
