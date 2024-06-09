#include "credits_player.hpp"

using namespace geode::prelude;

bool GlobedCreditsPlayer::init(const std::string_view name, const std::string_view nickname, int accountId, int userId, const GlobedSimplePlayer::Icons& icons) {
    if (!CCNode::init()) return false;

    this->accountId = accountId;

    auto* sp = Build<GlobedSimplePlayer>::create(icons)
        .anchorPoint({0.5f, 0.5f})
        .zOrder(0)
        .parent(this)
        .collect();

    // shadow
    auto* shadow = Build<CCSprite>::createSpriteName("shadow-thing-idk.png"_spr)
        .scale(0.55f)
        .pos(0.f, -13.f)
        .zOrder(-1)
        .parent(this)
        .collect();

    // name
    CCMenuItemSpriteExtra* nameLabel;
    auto menu = Build<CCLabelBMFont>::create(std::string(nickname).c_str(), "goldFont.fnt")
        .scale(0.45f)
        .limitLabelWidth(50.f, 0.45f, 0.05f)
        .intoMenuItem([accountId] {
            ProfilePage::create(accountId, false)->show();
        })
        .pos(0.f, 24.f)
        .store(nameLabel)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(this)
        .collect();

    GameLevelManager::get()->storeUserName(userId, accountId, gd::string(std::string(name)));

    float width = sp->getScaledContentSize().width * 1.1f;
    float height = sp->getScaledContentSize().height * 1.1f + nameLabel->getScaledContentSize().height;

    this->setContentSize({width, height});

    CCPoint delta = CCPoint{width, height} / 2;
    sp->setPosition(sp->getPosition() + delta);
    shadow->setPosition(shadow->getPosition() + delta);
    menu->setPosition(menu->getPosition() + delta);

    return true;
}

GlobedCreditsPlayer* GlobedCreditsPlayer::create(const std::string_view name, const std::string_view nickname, int accountId, int userId, const GlobedSimplePlayer::Icons& icons) {
    auto ret = new GlobedCreditsPlayer;
    if (ret->init(name, nickname, accountId, userId, icons)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}