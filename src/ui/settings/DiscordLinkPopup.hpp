#pragma once

#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/prelude.hpp>
#include <ui/BasePopup.hpp>
#include <ui/misc/LazyPlayerIcon.hpp>

#include <Geode/ui/NineSlice.hpp>
#include <cue/LoadingCircle.hpp>

namespace globed {

class DiscordLinkPopup : public BasePopup {
public:
    static DiscordLinkPopup* create();

protected:
    CCNode* m_playerCard;
    geode::NineSlice* m_background;
    Button* m_discordBtn = nullptr;
    Button* m_activeBtn = nullptr;
    CCNode* m_statusContainer = nullptr;
    CCNode* m_dataContainer = nullptr;
    Label* m_statusLabel = nullptr;
    Label* m_nameLabel = nullptr;
    Label* m_idLabel = nullptr;
    Label* m_waitingLabel1 = nullptr;
    Label* m_waitingLabel2 = nullptr;
    CCNode* m_waitingContainer = nullptr;
    LazyPlayerIcon* m_playerIcon;
    geode::LazySprite* m_avatar = nullptr;
    bool m_activelyWaiting = false;
    bool m_linked = false;

    MessageListener<msg::DiscordLinkStateMessage> m_stateListener;
    MessageListener<msg::DiscordOauthUrlMessage> m_oauthListener;
    MessageListener<msg::DiscordUnlinkResultMessage> m_unlinkListener;

    bool init() override;
    void onClose(CCObject*) override;

    void onStateLoaded(uint64_t id, const std::string& username, const std::string& avatarUrl);
    // void onAttemptReceived(uint64_t id, const std::string& username, const std::string& avatarUrl);
    void onOauthUrlReceived(ZStringView url);

    void addLinkingText();

    void startWaitingForRefresh();
    void startWaitingForUnlink();
    void requestState(float dt);

};

}