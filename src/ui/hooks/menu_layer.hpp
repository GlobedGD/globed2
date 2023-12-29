#pragma once
#include <defs.hpp>

#include <Geode/modify/MenuLayer.hpp>
#include <UIBuilder.hpp>

#include <net/network_manager.hpp>
#include <ui/menu/main/globed_menu_layer.hpp>

using namespace geode::prelude;

class $modify(HookedMenuLayer, MenuLayer) {
    bool btnActive = false;
    CCMenuItemSpriteExtra* globedBtn = nullptr;

    bool init() {
        if (!MenuLayer::init()) return false;

        this->updateGlobedButton();

        CCScheduler::get()->scheduleSelector(schedule_selector(HookedMenuLayer::maybeUpdateButton), this, 0.1f, false);

        return true;
    }

    void updateGlobedButton() {
        if (m_fields->globedBtn) m_fields->globedBtn->removeFromParent();

        auto menu = this->getChildByID("bottom-menu");

        Build<CircleButtonSprite>(CircleButtonSprite::createWithSpriteFrameName(
            "miniSkull_001.png",
            1.f,
            m_fields->btnActive ? CircleBaseColor::Cyan : CircleBaseColor::Green,
            CircleBaseSize::MediumAlt
            ))
            .intoMenuItem([](auto) {
                auto accountId = GJAccountManager::sharedState()->m_accountID;
                if (accountId <= 0) {
                    FLAlertLayer::create("Notice", "You need to be signed into a <cg>Geometry Dash account</c> in order to play online.", "Ok")->show();
                    return;
                }

                CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, GlobedMenuLayer::scene()));
            })
            .id("main-menu-button"_spr)
            .parent(menu)
            .store(m_fields->globedBtn);

        menu->updateLayout();
    }

    void maybeUpdateButton(float) {
        bool authenticated = NetworkManager::get().established();
        if (authenticated != m_fields->btnActive) {
            m_fields->btnActive = authenticated;
            this->updateGlobedButton();
        }
    }
};