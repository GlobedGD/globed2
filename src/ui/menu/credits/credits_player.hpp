#pragma once
#include <defs/geode.hpp>

#include <ui/general/simple_player.hpp>

// simpleplayer plus shadow and name
class GlobedCreditsPlayer : public cocos2d::CCNode {
public:

    static GlobedCreditsPlayer* create(std::string_view name, std::string_view nickname, int accountId, int userId, const GlobedSimplePlayer::Icons& icons);

private:
    bool init(std::string_view name, std::string_view nickname, int accountId, int userId, const GlobedSimplePlayer::Icons& icons);

    int accountId;
};
