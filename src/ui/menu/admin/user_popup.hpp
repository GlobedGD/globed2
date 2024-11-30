#pragma once
#include <defs/all.hpp>

#include <Geode/ui/TextInput.hpp>

#include <data/types/admin.hpp>
#include <data/types/gd.hpp>
#include <data/packets/packet.hpp>
#include <ui/general/loading_circle.hpp>

class AdminUserPopup : public geode::Popup<const UserEntry&, const std::optional<PlayerRoomPreviewAccountData>&>, public UserInfoDelegate {
public:
    static constexpr float POPUP_WIDTH = 300.f;
    static constexpr float POPUP_HEIGHT = 190.f;

    static AdminUserPopup* create(const UserEntry& userEntry, const std::optional<PlayerRoomPreviewAccountData>& accountData);
    void showLoadingPopup();

private:
    class WaitForResponsePopup;
    friend class AdminPunishUserPopup;

    UserEntry userEntry;
    std::optional<PlayerRoomPreviewAccountData> accountData;
    GJUserScore* userScore = nullptr;
    Ref<BetterLoadingCircle> loadingCircle = nullptr;
    ColorChannelSprite* nameColorSprite = nullptr;
    cocos2d::CCLabelBMFont* banDurationText = nullptr;
    geode::TextInput *inputReason = nullptr;
    geode::TextInput *inputAdminPassword = nullptr;
    cocos2d::CCMenu* nameLayout;
    cocos2d::CCMenu* rootMenu;
    Ref<CCMenuItemSpriteExtra> roleModifyButton, banButton, muteButton;
    std::shared_ptr<Packet> queuedPacket;
    bool entryWasEmpty = false;
    bool waitingForUpdateUsername = false;

    bool setup(const UserEntry& userEntry, const std::optional<PlayerRoomPreviewAccountData>& accountData) override;
    void onProfileLoaded();
    void onColorSelected(cocos2d::ccColor3B);
    void recreateRoleModifyButton();
    void createBanAndMuteButtons();

    cocos2d::ccColor3B getCurrentNameColor();

    void sendUpdateUser();

    void onClose(cocos2d::CCObject*) override;
    void removeLoadingCircle();
    void refreshFromUserEntry(UserEntry entry);

    void performAction(std::shared_ptr<Packet> packet);

    void getUserInfoFinished(GJUserScore* p0) override;
    void getUserInfoFailed(int p0) override;
    void userInfoChanged(GJUserScore* p0) override;
};
