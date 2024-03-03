#include "signup_layer.hpp"

#include "signup_popup.hpp"
#include <managers/settings.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedSignupLayer::init() {
    if (!CCLayer::init()) return false;

    auto listview = Build<ListView>::create(CCArray::create(), 0.f, LIST_WIDTH, LIST_HEIGHT).collect();

    auto listLayer = Build<GJListLayer>::create(listview, "Authentication", util::ui::BG_COLOR_BROWN, LIST_WIDTH, 220.f, 0)
        .zOrder(2)
        .anchorPoint(0.f, 0.f)
        .pos(0.f, 0.f)
        .parent(this)
        .collect();

    this->setContentSize(listLayer->getScaledContentSize());

    Build<ButtonSprite>::create("Login", "goldFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([](auto) {
            auto& gs = GlobedSettings::get();

            if (!gs.flags.seenSignupNoticev2) {
                geode::createQuickPopup("Notice", CONSENT_MESSAGE, "Cancel", "Submit", [&gs](auto, bool agreed){
                    if (agreed) {
                        gs.flags.seenSignupNoticev2 = true;
                        gs.save();
                        if (auto* popup = GlobedSignupPopup::create()) {
                            popup->show();
                        }
                    }
                });
            } else {
                if (auto* popup = GlobedSignupPopup::create()) {
                    popup->show();
                }
            }
        })
        .anchorPoint(0.5f, 0.5f)
        .pos(listLayer->m_listView->getScaledContentSize() / 2.f)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(listLayer);

    return true;
}

GlobedSignupLayer* GlobedSignupLayer::create() {
    auto ret = new GlobedSignupLayer;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}