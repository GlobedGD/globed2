#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include <UIBuilder.hpp>

#include <net/network_manager.hpp>
#include <ui/menu/main/globed_menu_layer.hpp>

using namespace geode::prelude;

class $modify(HookedMenuLayer, MenuLayer) {
    bool m_btnActive = false;
    CCMenuItemSpriteExtra* m_globedBtn = nullptr;

    bool init() {
        if (!MenuLayer::init()) return false;

        updateGlobedButton();
        
        CCScheduler::get()->scheduleSelector(schedule_selector(HookedMenuLayer::maybeUpdateButton), this, 0.1f, false);

        return true;
    }

    void updateGlobedButton() {
        if (m_fields->m_globedBtn) m_fields->m_globedBtn->removeFromParent();

        auto menu = this->getChildByID("bottom-menu");

        Build<CircleButtonSprite>(CircleButtonSprite::createWithSpriteFrameName(
            "miniSkull_001.png",
            1.f,
            m_fields->m_btnActive ? CircleBaseColor::Cyan : CircleBaseColor::Green,
            CircleBaseSize::MediumAlt
            ))
            .intoMenuItem([](CCObject*) {
                CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, GlobedMenuLayer::scene()));
            })
            .id("main-menu-button"_spr)
            .parent(menu)
            .store(m_fields->m_globedBtn);

        menu->updateLayout();
    }

    void maybeUpdateButton(float _) {
        bool authenticated = NetworkManager::get().authenticated();
        if (authenticated != m_fields->m_btnActive) {
            m_fields->m_btnActive = authenticated;
            updateGlobedButton();
        }
    }
};