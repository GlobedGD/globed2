#pragma once
#include <defs/all.hpp>

class ServerSwitcherPopup : public geode::Popup<> {
public:
    constexpr static float POPUP_WIDTH = 420.f;
    constexpr static float POPUP_HEIGHT = 280.f;
    constexpr static float LIST_WIDTH = 340.f;
    constexpr static float LIST_HEIGHT = 180.f;

    void reloadList();
    void close();

    static ServerSwitcherPopup* create();
protected:
    GJCommentListLayer* listLayer = nullptr;

    bool setup() override;
};