#include "UserActionsPopup.hpp"
#include <globed/core/PlayerCacheManager.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

bool UserActionsPopup::init(int accountId, CCArray* buttons) {
    if (!BasePopup::init(200.f, 90.f)) return false;

    m_accountId = accountId;

    auto& pcm = PlayerCacheManager::get();
    auto data = pcm.get(accountId);

    this->setTitle(data ? data->username : "Player");

    m_buttons = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f))
        .pos(this->fromCenter(0.f, -10.f))
        .parent(m_mainLayer);

    for (auto btn : CCArrayExt<CCNode>(buttons)) {
        m_buttons->addChild(btn);
    }

    m_buttons->updateLayout();

    return true;
}

UserActionsPopup* UserActionsPopup::create(int accountId, CCArray* buttons) {
    auto ret = new UserActionsPopup();
    if (ret->init(accountId, buttons)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

}
