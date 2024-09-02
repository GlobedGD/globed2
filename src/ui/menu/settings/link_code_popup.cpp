#include "link_code_popup.hpp"

#include <util/ui.hpp>
#include <data/packets/client/general.hpp>
#include <data/packets/server/general.hpp>
#include <net/manager.hpp>

using namespace geode::prelude;

bool LinkCodePopup::setup() {
    this->setTitle("Discord Link Code");

    auto popupLayout = util::ui::getPopupLayoutAnchored(m_size);

    Build<CCLabelBMFont>::create("Loading..", "bigFont.fnt")
        .scale(0.6f)
        .pos(popupLayout.center + CCSize{0.f, 15.f})
        .parent(m_mainLayer)
        .store(label);

    auto& nm = NetworkManager::get();
    nm.sendLinkCodeRequest();

    nm.addListener<LinkCodeResponsePacket>(this, [this](auto packet) {
        this->onLinkCodeReceived(packet->linkCode);
    });

    return true;
}

void LinkCodePopup::onLinkCodeReceived(uint32_t linkCode) {
    label->setScale(0.7f);
    label->setString(fmt::format("Code: {}", linkCode).c_str());

    Build<ButtonSprite>::create("Copy", "bigFont.fnt", "GJ_button_01.png", 0.7)
        .intoMenuItem([linkCode] {
            geode::utils::clipboard::write(std::to_string(linkCode));
            Notification::create("Copied to clipboard", NotificationIcon::Success, 1.f);
        })
        .pos(util::ui::getPopupLayoutAnchored(m_size).center - CCSize{0.f, 25.f})
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);
}

LinkCodePopup* LinkCodePopup::create() {
    auto ret = new LinkCodePopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return ret;
}