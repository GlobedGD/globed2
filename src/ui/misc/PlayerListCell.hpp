#pragma once
#include <ui/misc/NameLabel.hpp>

#include <Geode/Geode.hpp>
#include <cue/PlayerIcon.hpp>

namespace globed {

class PlayerListCell : public cocos2d::CCNode {
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
    std::string m_username;
    CCMenuItemSpriteExtra* m_usernameBtn;
    cue::PlayerIcon* m_cubeIcon;
    cocos2d::CCNode* m_leftContainer;
    cocos2d::CCMenu* m_rightMenu;
    NameLabel* m_nameLabel;

    bool init(
        int accountId,
        int userId,
        const std::string& username,
        const cue::Icons& icons,
        cocos2d::CCSize cellSize
    );

    bool initMyself(cocos2d::CCSize cellSize);
    void updateStuff(float dt);

    inline virtual bool customSetup() {
        return true;
    }
};

}
