#include "LinkRequestPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <modules/two-player/TwoPlayerModule.hpp>

#include <UIBuilder.hpp>
#include <cue/PlayerIcon.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

const CCSize LinkRequestPopup::POPUP_SIZE { 270.f, 160.f };

bool LinkRequestPopup::setup(int accountId, UserListPopup* popup) {
    auto gjbgl = GlobedGJBGL::get();
    auto rp = gjbgl->getPlayer(accountId);

    m_accountId = accountId;
    m_userListPopup = popup;

    this->setTitle("Link Request");
    m_title->setPosition(this->fromTop(14.f));

    m_username = rp && rp->isDataInitialized() ? rp->displayData().username : "Unknown";

    Build<CCLabelBMFont>::create(m_username.c_str(), "bigFont.fnt")
        .scale(0.6f)
        .pos(this->fromTop(35.f))
        .parent(m_mainLayer);

    // draw an icon
    cue::Icons icons{};
    if (rp) {
        auto& data = rp->displayData().icons;
        icons.type = IconType::Cube;
        icons.id = data.cube;
        icons.color1 = data.color1.asIdx();
        icons.color2 = data.color2.asIdx();
        icons.glowColor = data.glowColor.asIdx();
    }

    Build<cue::PlayerIcon>::create(icons)
        .pos(this->fromCenter(0.f, 10.f))
        .parent(m_mainLayer);

    m_loadingCircle = Build<cue::LoadingCircle>::create()
        .scale(0.4f)
        .pos(this->fromBottom(23.f))
        .parent(m_mainLayer);

    m_loadText = Build<CCLabelBMFont>::create("Waiting for confirmation..", "bigFont.fnt")
        .scale(0.4f)
        .pos(this->fromBottom(50.f))
        .visible(false)
        .parent(m_mainLayer);

    m_reqMenu = Build<CCMenu>::create()
        .layout(RowLayout::create()->setAutoScale(false))
        .ignoreAnchorPointForPos(false)
        .pos(this->fromBottom(30.f))
        .contentSize(220.f, 40.f)
        .parent(m_mainLayer);

    Build<ButtonSprite>::create("Player 1", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .scale(0.8f)
        .intoMenuItem([this] {
            this->link(false);
        })
        .parent(m_reqMenu);

    Build<ButtonSprite>::create("Player 2", "bigFont.fnt", "GJ_button_01.png", 0.7f)
        .scale(0.8f)
        .intoMenuItem([this] {
            this->link(true);
        })
        .parent(m_reqMenu);

    m_reqMenu->updateLayout();

    this->scheduleUpdate();

    return true;
}

void LinkRequestPopup::link(bool p2) {
    auto& mod = TwoPlayerModule::get();
    if (!mod.link(m_accountId, p2)) {
        return;
    }

    m_waiting = true;
    m_reqMenu->setVisible(false);
    m_loadingCircle->fadeIn();
    m_loadText->setVisible(true);
    m_startedWaiting = Instant::now();
}

void LinkRequestPopup::update(float dt) {
    auto& mod = TwoPlayerModule::get();

    if (mod.isLinked()) {
        globed::toastSuccess("Linked to {}", m_username);
        this->onClose(nullptr);
        return;
    }

    if (m_waiting && !mod.waitingForLink()) {
        // likely declined by another player
        this->onClose(nullptr);
        return;
    }

    // otherwise, just wait until timeout
    if (m_waiting && m_startedWaiting.elapsed() > Duration::fromSecs(15)) {
        this->onClose(nullptr);
        globed::alert("Error", "Player took <cy>too long to respond</c>, link attempt was <cr>cancelled</c>.");
        return;
    }
}

void LinkRequestPopup::onClose(CCObject* obj) {
    auto& mod = TwoPlayerModule::get();
    if (mod.waitingForLink()) {
        mod.cancelLink();
    }

    m_userListPopup->hardRefresh();

    BasePopup::onClose(obj);
}

}
