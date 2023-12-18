#include "signup_layer.hpp"

#include <UIBuilder.hpp>

#include "signup_popup.hpp"
#include <managers/settings.hpp>

using namespace geode::prelude;

bool GlobedSignupLayer::init() {
    if (!CCLayer::init()) return false;

    auto listview = Build<ListView>::create(CCArray::create(), 0.f, LIST_WIDTH, LIST_HEIGHT).collect();

    auto listLayer = Build<GJListLayer>::create(listview, "Authentication", ccc4(194, 114, 62, 255), LIST_WIDTH, 220.f)
        .zOrder(2)
        .anchorPoint(0.f, 0.f)
        .pos(0.f, 0.f)
        .parent(this)
        .collect();

    this->setContentSize(listLayer->getScaledContentSize());

    Build<ButtonSprite>::create("Login", "goldFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([](auto) {
            auto& gs = GlobedSettings::get();
            if (!gs.flags.seenSignupNotice) {
                geode::createQuickPopup("Notice", CONSENT_MESSAGE, "Cancel", "Ok", [&gs](auto, bool agreed){
                    if (agreed) {
                        gs.flags.seenSignupNotice = true;
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
        .pos(-listLayer->getScaledContentSize() / 4 - CCPoint{17.f, 10.f})
        .intoNewParent(CCMenu::create())
        .parent(listLayer);

    return true;
}

GlobedSignupLayer* GlobedSignupLayer::create() {
    auto ret = new GlobedSignupLayer;
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }

    CC_SAFE_DELETE(ret);
    return nullptr;
}