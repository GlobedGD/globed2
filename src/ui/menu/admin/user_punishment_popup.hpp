#pragma once

#include <defs/geode.hpp>

#include <data/types/user.hpp>

class UserPunishmentPopup : public geode::Popup<UserPunishment const&> {
protected:
    bool setup(UserPunishment const& punishment) override;

public:
    static UserPunishmentPopup* create(UserPunishment const& punishment);
    
};

class UserPunishmentCheckNode : public cocos2d::CCNode {
protected:
    bool init(UserPunishment const& punishment);
    void updatePopup(float dt);

    UserPunishment punishment;

public:
    static UserPunishmentCheckNode* create(UserPunishment const& punishment);
};