#include "UnreadBadge.hpp"

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

bool UnreadBadge::init(int count) {
    if (!CCNode::init()) return false;

    this->setAnchorPoint({0.5f, 0.5f});

    auto targetSize = CCSize{32.f, 32.f};
    this->setContentSize(targetSize);

    Build<CCSprite>::createSpriteName("geode.loader/updates-failed.png")
        .pos(targetSize / 2)
        .with([&](auto spr) {
            cue::rescaleToMatch(spr, targetSize);
        })
        .parent(this);

    Build<CCLabelBMFont>::create(std::to_string(count).c_str(), "bigFont.fnt")
        .scale(0.7f)
        .pos(targetSize / 2)
        .parent(this);

    return true;
}

UnreadBadge* UnreadBadge::create(int count) {
    auto ret = new UnreadBadge;
    if (ret->init(count)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}