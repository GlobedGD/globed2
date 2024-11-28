#include "admin_login_popup.hpp"

#include <managers/account.hpp>
#include <managers/error_queues.hpp>
#include <managers/settings.hpp>
#include <net/manager.hpp>
#include <util/misc.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool AdminLoginPopup::setup() {
    auto sizes = util::ui::getPopupLayoutAnchored(m_size);

    this->setTitle("Admin Login");

    Build<TextInput>::create(POPUP_WIDTH * 0.75f, "Password", "bigFont.fnt")
        .pos(sizes.center.width, sizes.center.height + 20.f)
        .parent(m_mainLayer)
        .store(passwordInput);

    passwordInput->setFilter(std::string(util::misc::STRING_PRINTABLE_INPUT));
    passwordInput->setPasswordMode(true);
    passwordInput->setMaxCharCount(32);

    auto* btnLayout = Build<CCMenu>::create()
        .pos(sizes.center.width, sizes.center.height - 20.f)
        .layout(RowLayout::create()->setGap(5.f)->setAutoScale(false))
        .parent(m_mainLayer)
        .collect();

    auto* loginBtn = Build<ButtonSprite>::create("Login", "bigFont.fnt", "geode.loader/GE_button_04.png", 0.8f)
        .intoMenuItem([this](auto) {
            auto& settings = GlobedSettings::get();

            auto password = this->passwordInput->getString();
            if (!password.empty()) {
                auto& nm = NetworkManager::get();
                if (!nm.established()) {
                    ErrorQueues::get().warn("not connected to a server");
                    return;
                }

                auto& gam = GlobedAccountManager::get();
                if (settings.admin.rememberPassword) {
                    gam.storeAdminPassword(password);
                }

                gam.storeTempAdminPassword(password);

                nm.send(AdminAuthPacket::create(password));
                this->onClose(this);
            }
        })
        .parent(btnLayout)
        .collect();

    auto* rememberPwd = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(AdminLoginPopup::onRememberPassword), 0.75f);
    rememberPwd->toggle(GlobedSettings::get().admin.rememberPassword);
    btnLayout->addChild(rememberPwd);

    Build<CCLabelBMFont>::create("Remember", "bigFont.fnt")
        .scale(0.5f)
        .parent(btnLayout);

    btnLayout->updateLayout();

    return true;
}

void AdminLoginPopup::onRememberPassword(cocos2d::CCObject* sender) {
    auto& settings = GlobedSettings::get();
    settings.admin.rememberPassword = !static_cast<CCMenuItemToggler*>(sender)->isOn();
}

AdminLoginPopup* AdminLoginPopup::create() {
    auto ret = new AdminLoginPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}