#pragma once
#include <defs/all.hpp>

#include <managers/web.hpp>

class AddServerPopup;

class ServerTestPopup : public geode::Popup<std::string_view, AddServerPopup*> {
public:
    constexpr static float POPUP_WIDTH = 180.f;
    constexpr static float POPUP_HEIGHT = 80.f;

    ~ServerTestPopup();
    void onTimeout();

    static ServerTestPopup* create(std::string_view, AddServerPopup* parent);

protected:
    AddServerPopup* parent;

    WebRequestManager::Listener requestListener;

    bool setup(std::string_view, AddServerPopup* parent) override;

    void cancelRequest();
    void requestCallback(typename WebRequestManager::Event* event);
};