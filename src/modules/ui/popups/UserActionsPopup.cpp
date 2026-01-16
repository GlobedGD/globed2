#include "UserActionsPopup.hpp"
#include <globed/core/PlayerCacheManager.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize UserActionsPopup::POPUP_SIZE{200.f, 90.f};

bool UserActionsPopup::setup(int accountId, CCArray *buttons)
{
    m_accountId = accountId;

    auto &pcm = PlayerCacheManager::get();
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

} // namespace globed
