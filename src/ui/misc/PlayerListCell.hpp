#pragma once
#include <globed/prelude.hpp>
#include <ui/misc/NameLabel.hpp>
#include <ui/misc/CellGradients.hpp>

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
        const std::optional<SpecialUserData>& sud,
        cocos2d::CCSize cellSize
    );

    static PlayerListCell* createMyself(CCSize cellSize);

    int m_accountId;
    int m_userId;
    std::string m_username;

protected:
    enum class Context {
        Menu, Ingame, Invites
    };

    CCMenuItemSpriteExtra* m_usernameBtn;
    cue::PlayerIcon* m_cubeIcon;
    CCNode* m_leftContainer;
    CCMenu* m_rightMenu;
    CCNode* m_gradient = nullptr;
    CCSprite* m_crownIcon = nullptr;
    CCSprite* m_friendIcon = nullptr;
    NameLabel* m_nameLabel;

    bool init(
        int accountId,
        int userId,
        const std::string& username,
        const cue::Icons& icons,
        const std::optional<SpecialUserData>& sud,
        CCSize cellSize
    );

    bool initMyself(CCSize cellSize);
    void updateStuff(float dt);
    void setGradient(CellGradientType type, bool blend = false);
    void initGradients(Context ctx = Context::Menu);

    inline virtual bool customSetup() {
        return true;
    }
};

}
