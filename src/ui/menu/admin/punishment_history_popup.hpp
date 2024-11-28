#pragma once

#include <Geode/ui//Popup.hpp>

#include <ui/general/loading_circle.hpp>

class UserPunishment;

class AdminPunishmentHistoryPopup : public geode::Popup<int32_t> {
public:
    static AdminPunishmentHistoryPopup* create(int32_t accountId);

protected:
    geode::Ref<BetterLoadingCircle> loadingCircle;

    class Cell;

    bool setup(int32_t accoutnId) override;

    void addPunishments(const std::vector<UserPunishment>& entries, const std::map<int, std::string>& usernames);
};
