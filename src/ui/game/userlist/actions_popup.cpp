#include "actions_popup.hpp"

#include <audio/voice_playback_manager.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <managers/profile_cache.hpp>
#include <managers/block_list.hpp>
#include <managers/settings.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedUserActionsPopup::setup(int accountId, CCArray* buttons) {
    this->accountId = accountId;

    auto& pcm = ProfileCacheManager::get();
    auto data = pcm.getData(accountId);

    std::string name = "Player";
    if (data.has_value()) {
        name = data->name;
    }

    this->setTitle(name);

    auto rlayout = util::ui::getPopupLayoutAnchored(m_size);

    Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f))
        .pos(rlayout.center - CCPoint{0.f, 10.f})
        .parent(m_mainLayer)
        .store(buttonLayout);

    auto pl = GlobedGJBGL::get();

    for (auto btn : CCArrayExt<CCNode*>(buttons)) {
        buttonLayout->addChild(btn);
    }

    buttonLayout->updateLayout();

    return true;
}

GlobedUserActionsPopup* GlobedUserActionsPopup::create(int accountId, CCArray* buttons) {
    auto ret = new GlobedUserActionsPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, accountId, buttons)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}