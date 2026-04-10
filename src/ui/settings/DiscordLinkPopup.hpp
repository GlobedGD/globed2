#pragma once

#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <ui/BasePopup.hpp>

#include <Geode/ui/NineSlice.hpp>
#include <cue/PlayerIcon.hpp>

namespace globed {

class DiscordLinkPopup : public BasePopup {
public:
    static DiscordLinkPopup* create();

protected:
    cocos2d::CCNode* m_playerCard;
    geode::NineSlice* m_background;
    CCMenuItemSpriteExtra* m_discordBtn = nullptr;
    CCMenuItemSpriteExtra* m_startBtn = nullptr;
    cocos2d::CCNode* m_statusContainer = nullptr;
    cocos2d::CCNode* m_dataContainer = nullptr;
    cocos2d::CCLabelBMFont* m_statusLabel = nullptr;
    cocos2d::CCLabelBMFont* m_nameLabel = nullptr;
    cocos2d::CCLabelBMFont* m_idLabel = nullptr;
    cocos2d::CCLabelBMFont* m_waitingLabel1 = nullptr;
    cocos2d::CCLabelBMFont* m_waitingLabel2 = nullptr;
    cue::PlayerIcon* m_playerIcon;
    geode::LazySprite* m_avatar = nullptr;
    bool m_activelyWaiting = false;
    bool m_linked = false;

    MessageListener<msg::DiscordLinkStateMessage> m_stateListener;
    MessageListener<msg::DiscordOauthUrlMessage> m_oauthListener;

    bool init() override;
    void onClose(CCObject*) override;

    void onStateLoaded(uint64_t id, const std::string& username, const std::string& avatarUrl);
    // void onAttemptReceived(uint64_t id, const std::string& username, const std::string& avatarUrl);
    void onOauthUrlReceived(geode::ZStringView url);

    void addLinkingText();

    void startWaitingForRefresh();
    void requestState(float dt);
};

}