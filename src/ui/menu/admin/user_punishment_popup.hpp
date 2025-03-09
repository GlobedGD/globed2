#pragma once

#include <defs/geode.hpp>

#include <data/types/user.hpp>

class UserPunishmentPopup : public geode::Popup<UserPunishment const&> {
protected:
    bool setup(UserPunishment const& punishment) override;

public:
    static UserPunishmentPopup* create(UserPunishment const& punishment);
};