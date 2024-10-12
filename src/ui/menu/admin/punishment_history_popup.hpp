#pragma once

#include <Geode/ui//Popup.hpp>

class AdminPunishmentHistoryPopup : public geode::Popup<int32_t> {
public:
    static AdminPunishmentHistoryPopup* create(int32_t accountId);

protected:
    bool setup(int32_t accoutnId) override;
};
