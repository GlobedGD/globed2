#pragma once

#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

class AdminPunishUserPopup : public geode::Popup<int32_t, bool> {
public:
    static AdminPunishUserPopup* create(int32_t accountId, bool isBan);

private:
    geode::TextInput *reasonInput, *daysInput, *hoursInput;
    std::map<std::chrono::seconds, CCMenuItemToggler*> durationButtons;
    std::chrono::seconds currentDuration{0};

    class CommonReasonPopup;
    friend class AdminPunishUserPopup::CommonReasonPopup;

    bool setup(int32_t accountId, bool isBan);
    void setDuration(std::chrono::seconds dur, bool inCallback = false);
    void setReason(const std::string& reason);
    void inputChanged();
    void submit();
};
