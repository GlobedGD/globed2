#pragma once

#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <ui/BasePopup.hpp>

#include <cue/PlayerIcon.hpp>

namespace globed {

class DiscordLinkPopup : public BasePopup<DiscordLinkPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

protected:
    cocos2d::CCNode* m_playerCard;
    cocos2d::extension::CCScale9Sprite* m_background;
    CCMenuItemSpriteExtra* m_discordBtn = nullptr;
    CCMenuItemSpriteExtra* m_startBtn = nullptr;
    cocos2d::CCNode* m_statusContainer = nullptr;
    cocos2d::CCLabelBMFont* m_statusLabel = nullptr;
    cocos2d::CCLabelBMFont* m_nameLabel = nullptr;
    cocos2d::CCLabelBMFont* m_idLabel = nullptr;
    cocos2d::CCLabelBMFont* m_waitingLabel1 = nullptr;
    cocos2d::CCLabelBMFont* m_waitingLabel2 = nullptr;
    cue::PlayerIcon* m_playerIcon;
    geode::LazySprite* m_avatar;
    bool m_activelyWaiting = false;

    std::optional<MessageListener<msg::DiscordLinkStateMessage>> m_stateListener;
    std::optional<MessageListener<msg::DiscordLinkAttemptMessage>> m_attemptListener;

    bool setup() override;
    void onClose(CCObject*) override;

    void onStateLoaded(uint64_t id, const std::string& username, const std::string& avatarUrl);
    void onAttemptReceived(uint64_t id, const std::string& username, const std::string& avatarUrl);

    void addLinkingText();

    void startWaitingForRefresh();
    void requestState(float dt);
};

}