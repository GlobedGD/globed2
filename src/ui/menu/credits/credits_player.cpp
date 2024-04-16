#include "credits_player.hpp"

using namespace geode::prelude;

bool GlobedCreditsPlayer::init(const std::string_view name, int accountId, int userId, const GlobedSimplePlayer::Icons& icons) {
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
    auto cl = Build<CCLabelBMFont>::create(std::string(name).c_str(), "goldFont.fnt")
        .scale(0.45f)
        .collect();

    cl->limitLabelWidth(55.f, 0.45f, 0.05f);

    CCMenuItemSpriteExtra* nameLabel;
    auto* menu = Build(CCMenuItemSpriteExtra::create(cl, this, menu_selector(GlobedCreditsPlayer::onNameClicked)))
        .pos(0.f, 24.f)
        .store(nameLabel)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(this)
        .collect();

    // he likes being an exception
    if (name == "ninXout") {
        GameLevelManager::get()->storeUserName(userId, accountId, "Awesomeoverkill");
    } else {
        GameLevelManager::get()->storeUserName(userId, accountId, gd::string(std::string(name)));
    }

    float width = sp->getScaledContentSize().width * 1.1f;
    float height = sp->getScaledContentSize().height * 1.1f + nameLabel->getScaledContentSize().height;

    this->setContentSize({width, height});

    CCPoint delta = CCPoint{width, height} / 2;
    sp->setPosition(sp->getPosition() + delta);
    shadow->setPosition(shadow->getPosition() + delta);
    menu->setPosition(menu->getPosition() + delta);

    return true;
}

void GlobedCreditsPlayer::onNameClicked(cocos2d::CCObject*) {
    ProfilePage::create(accountId, false)->show();
}

GlobedCreditsPlayer* GlobedCreditsPlayer::create(const std::string_view name, int accountId, int userId, const GlobedSimplePlayer::Icons& icons) {
    auto ret = new GlobedCreditsPlayer;
    if (ret->init(name, accountId, userId, icons)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}