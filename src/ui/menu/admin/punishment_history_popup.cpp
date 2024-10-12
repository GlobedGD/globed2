#include "punishment_history_popup.hpp"

bool AdminPunishmentHistoryPopup::setup(int32_t accountId) {
    return true;
}

AdminPunishmentHistoryPopup* AdminPunishmentHistoryPopup::create(int32_t accountId) {
    auto ret = new AdminPunishmentHistoryPopup;
    if (ret->initAnchored(340.f, 240.f, accountId)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
