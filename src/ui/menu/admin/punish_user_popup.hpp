#pragma once

#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <data/types/user.hpp>

#include <asp/time/Duration.hpp>

class AdminUserPopup;

class AdminPunishUserPopup : public geode::Popup<AdminUserPopup*, int32_t, PunishmentType, std::optional<UserPunishment>> {
public:
    static AdminPunishUserPopup* create(AdminUserPopup* popup, int32_t accountId, PunishmentType type, std::optional<UserPunishment> punishment);

private:
    int32_t accountId;
    PunishmentType type;
    AdminUserPopup* parentPopup;
    geode::TextInput *reasonInput, *daysInput, *hoursInput;
    std::map<asp::time::Duration, CCMenuItemToggler*> durationButtons;
    asp::time::Duration currentDuration{};

    std::optional<UserPunishment> punishment;

    class CommonReasonPopup;
    friend class AdminPunishUserPopup::CommonReasonPopup;

    bool setup(AdminUserPopup* popup, int32_t accountId, PunishmentType type, std::optional<UserPunishment> punishment);
    void setDuration(asp::time::Duration dur, bool inCallback = false);
    void setReason(const std::string& reason);
    void inputChanged();
    void submit();
    void submitRemoval();
};
