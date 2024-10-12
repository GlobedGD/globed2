#pragma once

#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <data/types/user.hpp>

class AdminUserPopup;

class AdminPunishUserPopup : public geode::Popup<AdminUserPopup*, int32_t, bool, std::optional<UserPunishment>> {
public:
    static AdminPunishUserPopup* create(AdminUserPopup* popup, int32_t accountId, bool isBan, std::optional<UserPunishment> punishment);

private:
    int32_t accountId;
    bool isBan;
    AdminUserPopup* parentPopup;
    geode::TextInput *reasonInput, *daysInput, *hoursInput;
    std::map<std::chrono::seconds, CCMenuItemToggler*> durationButtons;
    std::chrono::seconds currentDuration{0};

    std::optional<UserPunishment> punishment;

    class CommonReasonPopup;
    friend class AdminPunishUserPopup::CommonReasonPopup;

    bool setup(AdminUserPopup* popup, int32_t accountId, bool isBan, std::optional<UserPunishment> punishment);
    void setDuration(std::chrono::seconds dur, bool inCallback = false);
    void setReason(const std::string& reason);
    void inputChanged();
    void submit();
    void submitRemoval();
};
