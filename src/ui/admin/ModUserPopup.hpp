#pragma once

#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <ui/BasePopup.hpp>

#include <cue/LoadingCircle.hpp>
#include <cue/Util.hpp>

namespace globed {

class ModUserPopup : public BasePopup<ModUserPopup, int>, public UserInfoDelegate, public LevelManagerDelegate {
public:
    static const cocos2d::CCSize POPUP_SIZE;

    void startLoadingProfile(const std::string& query, bool isId);
    void startLoadingProfile(int id);

protected:
    cue::LoadingCircle* m_loadCircle;
    std::optional<MessageListener<msg::AdminFetchResponseMessage>> m_listener;
    std::optional<MessageListener<msg::AdminResultMessage>> m_resultListener;
    std::string m_query;
    bool m_queryIsId = false;
    cocos2d::CCMenu* m_nameLayout = nullptr;
    cocos2d::CCMenu* m_rootMenu = nullptr;
    CCMenuItemSpriteExtra* m_roleModifyButton = nullptr;
    CCMenuItemSpriteExtra* m_banButton = nullptr;
    CCMenuItemSpriteExtra* m_muteButton = nullptr;
    CCMenuItemSpriteExtra* m_roomBanButton = nullptr;

    struct Data {
        int accountId = 0;
        bool whitelisted = false;
        std::vector<uint8_t> roles;
        std::optional<UserPunishment> activeBan;
        std::optional<UserPunishment> activeRoomBan;
        std::optional<UserPunishment> activeMute;
        size_t punishmentCount;
    };

    std::optional<Data> m_data;
    GJUserScore* m_score = nullptr;

    bool setup(int accountId) override;
    void initUi();
    void recreateRoleButton();
    void createMuteAndBanButtons();
    void fullRefresh();
    void showPunishmentPopup(UserPunishmentType type);
    void sendUpdateMessage();

    void onLoaded(const msg::AdminFetchResponseMessage& msg);
    void onUserInfoLoaded(geode::Result<GJUserScore*> res, bool sendUpdate = true);

    void getUserInfoFinished(GJUserScore* p0) override;
    void getUserInfoFailed(int p0) override;

    void loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int p2) override;
    void loadLevelsFailed(char const* key, int p1) override;
};

}
