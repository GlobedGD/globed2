#pragma once

#include <defs/geode.hpp>

#include <data/types/user.hpp>

class UserPunishmentPopup : public geode::Popup<geode::SimpleTextArea*, int64_t, bool> {
protected:
    bool setup(geode::SimpleTextArea* textarea, int64_t expiresAt, bool isBan) override;
    bool initCustomSize(std::string_view reason, int64_t expiresAt, bool isBan);

    void onClose(cocos2d::CCObject* a) override;

public:
    static UserPunishmentPopup* create(std::string_view reason, int64_t expiresAt, bool isBan);
};