#include "chat_cell.hpp"

#include "chatlist.hpp"
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

using namespace geode::prelude;

PlayerAccountData getAccountData(int id) {
    if (ProfileCacheManager::get().getData(id)) return ProfileCacheManager::get().getData(id).value();
    if (id == GJAccountManager::sharedState()->m_accountID) return ProfileCacheManager::get().getOwnAccountData();
    return PlayerAccountData::DEFAULT_DATA;
}

bool GlobedUserChatCell::init(const std::string& username, int accid, const std::string& messageText) {
    if (!CCLayerColor::init())
        return false;

    user = username;
    accountId = accid;

    auto GAM = GJAccountManager::sharedState();

    this->setOpacity(50);
    this->setContentSize(ccp(290, CELL_HEIGHT));
    this->setAnchorPoint(ccp(0, 0));

    auto* playerBundle = Build<CCMenu>::create()
        .pos(0.f + 2.f, CELL_HEIGHT - 2.f)
        .anchorPoint(0.f, 1.f)
        .layout(RowLayout::create()
            ->setGap(4.f)
            ->setGrowCrossAxis(true)
            ->setCrossAxisReverse(true)
            ->setAutoScale(false)
            ->setAxisAlignment(AxisAlignment::Start)
        )
        .parent(this)
        .collect();

    Build<GlobedSimplePlayer>::create(getAccountData(accid).icons)
        .scale(0.475f)
        .id("playericon")
        .zOrder(-1)
        .parent(playerBundle);

    auto* nameLabel = Build<CCLabelBMFont>::create(username.c_str(), "goldFont.fnt")
        .scale(0.5f)
        .id("playername")
        .zOrder(2)
        .collect();

    PlayerAccountData data = getAccountData(accid);

    CCSprite* badgeIcon = nullptr;
	if (data.specialUserData.has_value()) {
		badgeIcon = util::ui::createBadgeIfSpecial(data.specialUserData->nameColor);
        badgeIcon->setPosition(ccp(nameLabel->getPositionX() + nameLabel->getScaledContentSize().width / 2.f + 13.5f, nameLabel->getPositionY()));
        badgeIcon->setZOrder(1);
        playerBundle->addChild(badgeIcon);
    }

    auto* usernameButton = Build<CCMenuItemSpriteExtra>::create(nameLabel, this, menu_selector(GlobedUserChatCell::onUser))
        .pos(3.f, 35.f)
        .zOrder(0)
        .scaleMult(1.1f)
        .parent(playerBundle)
        .collect();

    playerBundle->updateLayout();

    // set the zorder of the button to be the highest, so that when you hold it, the badge and playre icon are behind
    // this also *must* be called after updateLayout().
    usernameButton->setZOrder(10);

    auto messageTextLabel = CCLabelBMFont::create(messageText.c_str(), "chatFont.fnt");

    messageTextLabel->setPosition(4, 17);
    messageTextLabel->limitLabelWidth(260.0f, 0.8f, 0.0f);
    //messageTextLabel->setScale(.65);
    messageTextLabel->setAnchorPoint(ccp(0, 0.5));
    messageTextLabel->setID("message-text");

    this->addChild(messageTextLabel);

    return true;
}

void GlobedUserChatCell::onUser(CCObject* sender) {
    ProfilePage::create(accountId, GJAccountManager::sharedState()->m_accountID == accountId)->show();
}

GlobedUserChatCell* GlobedUserChatCell::create(const std::string& username, int aid, const std::string& messageText) {
    auto* ret = new GlobedUserChatCell;
    if (ret->init(username, aid, messageText)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
