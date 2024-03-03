#include "intermediary_loading_popup.hpp"

bool IntermediaryLoadingPopup::setup(CallbackFn onInit, CallbackFn onCleanup) {
    circle = LoadingCircle::create();
    circle->setParentLayer(m_mainLayer);
    circle->setScale(0.75f);
    circle->show();

    callbackCleanup = onCleanup;
    onInit(this);

    return true;
}

void IntermediaryLoadingPopup::onClose(cocos2d::CCObject* sender) {
    circle->fadeAndRemove();
    callbackCleanup(this);
    Popup::onClose(sender);
}

IntermediaryLoadingPopup* IntermediaryLoadingPopup::create(CallbackFn onInit, CallbackFn onCleanup) {
    auto ret = new IntermediaryLoadingPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT, onInit, onCleanup)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}