#include "chat_cell.hpp"

#include <audio/voice_playback_manager.hpp>
#include <data/packets/client/admin.hpp>
#include <data/packets/server/admin.hpp>
#include <managers/block_list.hpp>
#include <managers/settings.hpp>
#include <managers/profile_cache.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <hooks/gjgamelevel.hpp>
#include <ui/menu/admin/user_popup.hpp>
#include <ui/general/ask_input_popup.hpp>
#include <ui/general/simple_player.hpp>
#include <ui/general/intermediary_loading_popup.hpp>
#include <util/format.hpp>
#include <util/ui.hpp>
#include <util/rng.hpp>

using namespace geode::prelude;

bool GlobedDeathCell::init(const std::string& username, int accid) {
    if (!CCLayerColor::init())
        return false;

    user = username;
    accountId = accid;

    auto GAM = GJAccountManager::sharedState();

    this->setContentSize(ccp(CELL_WIDTH, CELL_HEIGHT));
    this->setAnchorPoint(ccp(0, 0));

    // background
    Build<CCScale9Sprite>::create("square02_001.png")
        .contentSize(this->getContentSize() * 3.f)
        .scale(1.f / 3.f)
        .opacity(67)
        .zOrder(-1)
        .anchorPoint(0.f, 0.f)
        .parent(this);

    auto* playerBundle = Build<CCMenu>::create()
        .pos(0.f + 4.f, CELL_HEIGHT - 2.f)
        .anchorPoint(0.f, 1.f)
        .layout(RowLayout::create()
            ->setGap(4.f)
            ->setGrowCrossAxis(true)
            ->setCrossAxisReverse(true)
            ->setAutoScale(false)
            ->setCrossAxisOverflow(false)
            ->setAxisAlignment(AxisAlignment::Start)
        )
        .height(22.f)
        .parent(this)
        .collect();

    PlayerAccountData data = PlayerAccountData::DEFAULT_DATA;
    if (ProfileCacheManager::get().getData(accid)) data = ProfileCacheManager::get().getData(accid).value();
    if (accid == GJAccountManager::sharedState()->m_accountID) data = ProfileCacheManager::get().getOwnAccountData();

    Build<GlobedSimplePlayer>::create(data.icons)
        .scale(0.475f)
        .id("playericon")
        .zOrder(-1)
        .parent(playerBundle);

    auto* nameLabel = Build<CCLabelBMFont>::create(username.c_str(), "goldFont.fnt")
        .scale(0.5f)
        .id("playername")
        .zOrder(2)
        .collect();

    /*CCSprite* badgeIcon = util::ui::createBadgeIfSpecial(data.specialUserData);
    if (badgeIcon) {
        util::ui::rescaleToMatch(badgeIcon, util::ui::BADGE_SIZE);
        // TODO: fix this
        badgeIcon->setPosition(ccp(nameLabel->getPositionX() + nameLabel->getScaledContentSize().width / 2.f + 13.5f, nameLabel->getPositionY()));
        badgeIcon->setZOrder(1);
        playerBundle->addChild(badgeIcon);
    }*/

    auto* usernameButton = Build<CCMenuItemSpriteExtra>::create(nameLabel, this, menu_selector(GlobedDeathCell::onUser))
        .pos(3.f, 35.f)
        .zOrder(0)
        .scaleMult(1.1f)
        .parent(playerBundle)
        .collect();

    playerBundle->updateLayout();

    // set the zorder of the button to be the highest, so that when you hold it, the badge and player icon are behind
    // this also *must* be called after updateLayout().

    std::vector<std::string> MSGS = {
        "died.",
        "landed on a spike.",
        "thought they were noclipping.",
        "clicked too late.",
        "got a lag spike.",
        "tripped on a rock.",
        "forgot to jump.",
        "missed an input."
    };

    auto messageTextLabel = CCLabelBMFont::create(MSGS[util::rng::Random::get().generate<int>(0, MSGS.size() - 1)].c_str(), "bigFont.fnt");

    messageTextLabel->setPosition(4, 17);
    messageTextLabel->setScale(0.5f);
    messageTextLabel->limitLabelWidth(260.0f, 0.8f, 0.0f);
    messageTextLabel->setAnchorPoint(ccp(0, 0.5));
    messageTextLabel->setID("message-text");

    playerBundle->addChild(messageTextLabel);
    playerBundle->updateLayout();

    messageTextLabel->setScale(0.5f);
    playerBundle->updateLayout();

    runAction(CCSequence::create(CCDelayTime::create(5.f), CCFadeTo::create(0.5f, GLubyte(5)), nullptr));

    return true;
}

void GlobedDeathCell::onUser(CCObject* sender) {
    ProfilePage::create(accountId, GJAccountManager::sharedState()->m_accountID == accountId)->show();
}

GlobedDeathCell* GlobedDeathCell::create(const std::string& username, int aid) {
    auto* ret = new GlobedDeathCell;
    if (ret->init(username, aid)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
