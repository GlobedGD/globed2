#pragma once

#include <globed/core/data/Messages.hpp>
#include <globed/core/data/RoomSettings.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <ui/BasePopup.hpp>

#include <Geode/Geode.hpp>
#include <Geode/ui/TextInput.hpp>

namespace globed {

class LoadingPopup;

class CreateRoomPopup : public BasePopup<CreateRoomPopup> {
public:
    static const cocos2d::CCSize POPUP_SIZE;

private:
    geode::TextInput *m_nameInput;
    geode::TextInput *m_passcodeInput;
    geode::TextInput *m_playerLimitInput;
    CCNode *m_followerWrapper;
    CCNode *m_inputsWrapper;
    RoomSettings m_settings{};
    CCMenuItemSpriteExtra *m_safeModeBtn = nullptr;
    CCMenuItemToggler *m_twoPlayerBtn = nullptr;
    CCMenuItemToggler *m_deathlinkBtn = nullptr;
    CCMenuItemToggler *m_switcherooBtn = nullptr;
    CCMenuItemToggler *m_collisionBtn = nullptr;
    std::optional<MessageListener<msg::RoomStateMessage>> m_successListener;
    std::optional<MessageListener<msg::RoomCreateFailedMessage>> m_failListener;
    std::optional<MessageListener<msg::RoomBannedMessage>> m_bannedListener;
    LoadingPopup *m_loadingPopup = nullptr;

    bool setup() override;
    void onCheckboxToggled(cocos2d::CCObject *);
    void handleMutuallyExclusive(int activated);
    void setFollowerMode(bool enabled);
    void showSafeModePopup(bool firstTime);
    void showRoomNameWarnPopup(bool canName);

    void waitForResponse();
    void stopWaiting(std::optional<std::string> failReason);
};

} // namespace globed