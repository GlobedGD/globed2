#pragma once

#include <defs/geode.hpp>

#include <data/types/user.hpp>

class UserPunishmentPopup : public geode::Popup<std::string, int64_t, bool> {
protected:
    bool setup(std::string reason, int64_t expiresAt, bool isBan) override;

public:
    static UserPunishmentPopup* create(std::string reason, int64_t expiresAt, bool isBan);
    
};