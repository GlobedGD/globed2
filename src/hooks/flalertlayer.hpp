#pragma once
#include <defs/geode.hpp>
#include <Geode/modify/FLAlertLayer.hpp>

#include <asp/time/SystemTime.hpp>

struct GLOBED_DLL HookedFLAlertLayer : geode::Modify<HookedFLAlertLayer, FLAlertLayer> {
    struct Fields {
        bool blockClosing = false;
        asp::time::SystemTime blockClosingUntil;
    };

    $override
    void keyBackClicked() override;

    void blockClosingFor(asp::time::Duration duration);
};
