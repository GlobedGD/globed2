#pragma once

#include <Geode/Geode.hpp>
#include <cue/PlayerIcon.hpp>

namespace globed {

class PlayerListCell : public cocos2d::CCMenu {
public:
    static PlayerListCell* create(
        int accountId,
        int userId,
        const std::string& username,
        const cue::Icons& icons,
        cocos2d::CCSize cellSize
    );

    static PlayerListCell* createMyself(cocos2d::CCSize cellSize);

protected:
    int m_accountId;
    int m_userId;
    CCMenuItemSpriteExtra* m_usernameBtn;
    cue::PlayerIcon* m_cubeIcon;

    bool init(
        int accountId,
        int userId,
        const std::string& username,
        const cue::Icons& icons,
        cocos2d::CCSize cellSize
    );
};

}
