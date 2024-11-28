#include "intermediary_loading_popup.hpp"

#include <ui/general/loading_circle.hpp>

bool IntermediaryLoadingPopup::setup(CallbackFn&& onInit, CallbackFn&& onCleanup) {
    circle = BetterLoadingCircle::create();
    circle->addToLayer(m_mainLayer);
    circle->setScale(0.75f);
    circle->fadeIn();

    callbackCleanup = std::move(onCleanup);
    onInit(this);

    return true;
}

void IntermediaryLoadingPopup::onClose(cocos2d::CCObject* sender) {
    circle->fadeOut();
    callbackCleanup(this);
    Popup::onClose(sender);
}

IntermediaryLoadingPopup* IntermediaryLoadingPopup::create(CallbackFn&& onInit, CallbackFn&& onCleanup) {
    auto ret = new IntermediaryLoadingPopup;
    if (ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT, std::move(onInit), std::move(onCleanup))) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}