#include "admin_login_popup.hpp"

#include <net/network_manager.hpp>
#include <util/misc.hpp>

using namespace geode::prelude;

bool AdminLoginPopup::setup() {
    auto winSize = CCDirector::get()->getWinSize();
    auto center = winSize / 2;

    // i hate cocos ui
    CCSize bottom{center.width, center.height - m_size.height / 2};
    CCSize left{center.width - m_size.width / 2, center.height};
    CCSize right{center.width + m_size.width / 2, center.height};

    Build<InputNode>::create(POPUP_WIDTH * 0.75f, "password", "chatFont.fnt", std::string(util::misc::STRING_PRINTABLE_INPUT), 32)
        .pos(center.width, center.height + 30.f)
        .parent(m_mainLayer)
        .store(passwordInput);

    Build<ButtonSprite>::create("Login", "bigFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this](auto) {
            NetworkManager::get().send(AdminAuthPacket::create(this->passwordInput->getString()));
            this->onClose(this);
        })
        .pos(center.width, center.height - 20.f)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    return true;
}

AdminLoginPopup* AdminLoginPopup::create() {
    auto ret = new AdminLoginPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}