#include "ModLoginPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/util/FunctionQueue.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize ModLoginPopup::POPUP_SIZE { 280.f, 130.f };

bool ModLoginPopup::setup(std23::move_only_function<void()> callback) {
    m_callback = std::move(callback);

    this->setTitle("Mod Login");

    Build<TextInput>::create(POPUP_SIZE.width * 0.75f, "Password", "bigFont.fnt")
        .pos(this->fromCenter(0.f, 20.f))
        .parent(m_mainLayer)
        .store(m_passwordInput);

    m_passwordInput->setCommonFilter(CommonFilter::Any);
    m_passwordInput->setPasswordMode(true);
    m_passwordInput->setMaxCharCount(64);

    auto btnLayout = Build<CCMenu>::create()
        .pos(this->fromCenter(0.f, -20.f))
        .layout(RowLayout::create()->setAutoScale(false))
        .parent(m_mainLayer)
        .collect();


    auto* loginBtn = Build<ButtonSprite>::create("Login", "bigFont.fnt", "geode.loader/GE_button_04.png", 0.8f)
        .intoMenuItem([this](auto) {
            auto password = m_passwordInput->getString();

            if (!password.empty()) {
                auto& nm = NetworkManagerImpl::get();
                if (!nm.isConnected()) {
                    globed::alert("Error", "Not connected to a server");
                    return;
                }

                nm.storeModPassword(password);

                m_listener = nm.listen<msg::AdminResultMessage>([this](const auto& result) {
                    if (!result.success) {
                        globed::alertFormat("Error", "Failed to login: {}", result.error);
                    } else {
                        NetworkManagerImpl::get().markAuthorizedModerator();
                    }

                    this->stopWaiting();
                    return ListenerResult::Continue;
                });
                m_listener.value()->setPriority(-10000);

                nm.sendAdminLogin(password);

                this->wait();
            }
        })
        .parent(btnLayout)
        .collect();


    auto rememberPwd = CCMenuItemExt::createTogglerWithStandardSprites(0.75f, [this](auto toggler) {
        bool on = !toggler->isOn();
        globed::setting<bool>("core.mod.remember-password") = on;
    });

    rememberPwd->toggle(globed::setting<bool>("core.mod.remember-password"));
    btnLayout->addChild(rememberPwd);

    Build<CCLabelBMFont>::create("Remember", "bigFont.fnt")
        .scale(0.5f)
        .parent(btnLayout);

    btnLayout->updateLayout();

    return true;
}

void ModLoginPopup::wait() {
    m_loadPopup = LoadingPopup::create();
    m_loadPopup->setTitle("Logging in...");
    m_loadPopup->show();
}

void ModLoginPopup::stopWaiting() {
    m_loadPopup->forceClose();

    FunctionQueue::get().queue(std::move(m_callback));

    this->onClose(nullptr);
}

}