#include "overlay_cell.hpp"

#include <util/ui.hpp>

#include <ui/general/simple_player.hpp>
#include <managers/profile_cache.hpp>

using namespace util::ui;

bool ChatOverlayCell::init(int id, const std::string& message) {
    if (!CCNode::init()) return false;

    PlayerAccountData data = PlayerAccountData::DEFAULT_DATA;
    if (ProfileCacheManager::get().getData(id)) data = ProfileCacheManager::get().getData(id).value();
    if (id == GJAccountManager::sharedState()->m_accountID) data = ProfileCacheManager::get().getOwnAccountData();

    Build<CCMenu>::create()
        .parent(this)
        .store(nodeWrapper);

    nodeWrapper->setLayout(RowLayout::create()->setGap(5.f)->setAutoScale(false));

    // icon
    auto playerIcon = Build<GlobedSimplePlayer>::create(data.icons)
        .scale(0.45f)
        .pos(0.f, 0.f)
        .anchorPoint(0.f, 0.f)
        .parent(nodeWrapper)
        .collect();

    // username
    auto nameLabel = Build<CCLabelBMFont>::create(data.name.c_str(), "goldFont.fnt")
        .scale(0.35f)
        .parent(nodeWrapper)
        .collect();

    // message text
    Build<CCLabelBMFont>::create(message.c_str(), "bigFont.fnt")
        .scale(0.5f)
        .parent(nodeWrapper)
        .store(messageText);

    const float heightMult = 1.3f;
    const float widthMult = 1.1f;

    nodeWrapper->setContentWidth((playerIcon->getScaledContentSize().width + 5.f + nameLabel->getScaledContentSize().width + 5.f + messageText->getScaledContentSize().width));
    nodeWrapper->setContentHeight(playerIcon->getScaledContentSize().height * heightMult);
    nodeWrapper->updateLayout();

    // background
    auto sizeScale = 4.f;
    cc9s = Build<CCScale9Sprite>::create("square02_001.png")
        .contentSize(nodeWrapper->getScaledContentSize() * sizeScale + CCPoint{37.f, 25.f})
        .scaleX(1.f / sizeScale)
        .scaleY(1.f / sizeScale)
        .pos((nodeWrapper->getContentWidth() / 2.f) * -1, -8.f)
        .opacity(80)
        .zOrder(-1)
        .anchorPoint(0.f, 0.f)
        .parent(this)
        .collect();

    this->setContentSize(cc9s->getScaledContentSize());

    nodeWrapper->setPosition({this->getScaledContentSize().width - nodeWrapper->getScaledContentSize().width - 5.f, 3.5f});

    this->schedule(schedule_selector(ChatOverlayCell::updateChat));

    return true;
}

void ChatOverlayCell::updateChat(float dt) {
    // animations on a budget
    delta -= dt;
    if (delta <= 4.f) {
        // if on screen for more than 5 seconds
        nodeWrapper->setOpacity(255.f + (255.f / (-1 * delta)));
        cc9s->setOpacity(80.f + (80.f / (-1 * delta)));
        if (delta <= 1.4f) this->removeFromParentAndCleanup(true); // done with 8 seconds on screen
    }
}

void ChatOverlayCell::forceRemove() {
    // called when theres too many messages to wait 8 seconds for
    this->removeFromParentAndCleanup(true);
}

ChatOverlayCell* ChatOverlayCell::create(int id, const std::string& message) {
    auto ret = new ChatOverlayCell;
    if (ret->init(id, message)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}