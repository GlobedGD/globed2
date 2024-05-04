#include "overlay.hpp"

#include <hooks/gjbasegamelayer.hpp>
#include <managers/profile_cache.hpp>

using namespace geode::prelude;

bool GlobedChatOverlay::init() {
    if (!CCNode::init()) return false;

    this->setLayout(
        ColumnLayout::create()
            ->setGap(3.f)
            ->setAxisAlignment(AxisAlignment::Start)
            ->setCrossAxisAlignment(AxisAlignment::Start)
            ->setAxisReverse(true)
    );

    this->setContentHeight(CCDirector::get()->getWinSize().height);

    return true;
}

void GlobedChatOverlay::addMessage(int accountId, const std::string& message) {
    auto* cell = ChatOverlayCell::create(accountId, message);
    overlayCells.push_back(cell);
    this->addChild(cell);

    this->updateLayout();
}

GlobedChatOverlay* GlobedChatOverlay::create() {
    auto ret = new GlobedChatOverlay;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}