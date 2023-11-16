#include "signup_layer.hpp"

#include "signup_popup.hpp"

#include <UIBuilder.hpp>

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

    // Build<CCLabelBMFont>::create("In order to play on the servers, you have to verify your account.\nThis procedure requires no effort from your side and has to be done only once, but it may take a few seconds to complete.", "chatFont.fnt")
    //     .zOrder(10)
    //     .pos(25.f, 25.f)
    //     .scale(0.65f)
    //     .parent(listLayer);

    Build<ButtonSprite>::create("Login", "goldFont.fnt", "GJ_button_01.png", 0.8f)
        .intoMenuItem([this](auto) {
            GlobedSignupPopup::create()->show();
        })
        .anchorPoint(0.5f, 0.5f)
        .pos(-listLayer->getScaledContentSize() / 4 - CCPoint{17.f, 10.f})
        .intoNewParent(CCMenu::create())
        .parent(listLayer);

    return true;
}