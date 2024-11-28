#pragma once
#include <defs/all.hpp>

class BetterLoadingCircle;

class IntermediaryLoadingPopup : public geode::Popup<std::function<void(IntermediaryLoadingPopup*)>&&, std::function<void(IntermediaryLoadingPopup*)>&&> {
public:
    using CallbackFn = std::function<void(IntermediaryLoadingPopup*)>;

    static constexpr float POPUP_WIDTH = 160.f;
    static constexpr float POPUP_HEIGHT = 80.f;

    void onClose(cocos2d::CCObject* sender) override;

    static IntermediaryLoadingPopup* create(CallbackFn&& onInit, CallbackFn&& onCleanup);

private:
    BetterLoadingCircle* circle;
    CallbackFn callbackCleanup;

    bool setup(CallbackFn&& onInit, CallbackFn&& onCleanup) override;
};