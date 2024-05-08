#include "unread_badge.hpp"

#include <util/ui.hpp>

using namespace geode::prelude;

bool UnreadMessagesBadge::init(int count) {
    if (!CCNode::init()) return false;

    this->setAnchorPoint({0.5f, 0.5f});

    auto targetSize = CCSize{32.f, 32.f};
    this->setContentSize(targetSize);

    Build<CCSprite>::createSpriteName("geode.loader/updates-failed.png")
        .pos(targetSize / 2)
        .with([&](auto spr) {
            util::ui::rescaleToMatch(spr, targetSize);
        })
        .parent(this);

    Build<CCLabelBMFont>::create(std::to_string(count).c_str(), "bigFont.fnt")
        .scale(0.7f)
        .pos(targetSize / 2)
        .parent(this);

    return true;
}

UnreadMessagesBadge* UnreadMessagesBadge::create(int count) {
    auto ret = new UnreadMessagesBadge;
    if (ret->init(count)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}