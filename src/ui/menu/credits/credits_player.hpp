#pragma once
#include <defs/geode.hpp>

#include <ui/general/simple_player.hpp>

// simpleplayer plus shadow and name
class GlobedCreditsPlayer : public cocos2d::CCNode {
public:

    static GlobedCreditsPlayer* create(const std::string_view name, int accountId, int userId, const GlobedSimplePlayer::Icons& icons);

private:
    bool init(const std::string_view name, int accountId, int userId, const GlobedSimplePlayer::Icons& icons);

    void onNameClicked(cocos2d::CCObject*);

    int accountId;
};
