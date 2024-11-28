#pragma once
#include <defs/all.hpp>

#include <Geode/ui/TextInput.hpp>

class ServerSwitcherPopup;

class DirectConnectionPopup : public geode::Popup<ServerSwitcherPopup*> {
public:
    constexpr static float POPUP_WIDTH = 420.f;
    constexpr static float POPUP_HEIGHT = 280.f;

    static DirectConnectionPopup* create(ServerSwitcherPopup* parent);
protected:
    geode::TextInput* addressNode;
    ServerSwitcherPopup* parent;

    bool setup(ServerSwitcherPopup* popup) override;
};