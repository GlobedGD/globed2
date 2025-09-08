#pragma once

#include <globed/core/data/UserPunishment.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <ui/BasePopup.hpp>
#include <ui/misc/LoadingPopup.hpp>

#include <asp/time/Duration.hpp>
#include <Geode/Geode.hpp>

namespace globed {

class ModUserPopup;

class ModPunishPopup : public BasePopup<ModPunishPopup, int, UserPunishmentType, std::optional<UserPunishment>> {
public:
    static const cocos2d::CCSize POPUP_SIZE;
    using Callback = std::function<void()>;

    void setCallback(Callback&& cb);

protected:
    int m_accountId;
    UserPunishmentType m_type;
    std::optional<UserPunishment> m_punishment;
    Callback m_callback;

    geode::TextInput *m_reasonInput, *m_daysInput, *m_hoursInput;
    std::map<asp::time::Duration, CCMenuItemToggler*> m_durationButtons;
    asp::time::Duration m_currentDuration{};

    std::optional<MessageListener<msg::AdminResultMessage>> m_listener;
    LoadingPopup* m_loadPopup = nullptr;

    bool setup(int accountId, UserPunishmentType type, std::optional<UserPunishment> pun) override;
    void setDuration(asp::time::Duration dur, bool inCallback = false);
    void setReason(const std::string& reason);
    void inputChanged();
    void submit();
    void submitRemoval();

    void startWaiting();
    void stopWaiting(const msg::AdminResultMessage& msg);
};

}