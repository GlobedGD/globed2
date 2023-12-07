#pragma once
#include <defs.hpp>

#include <net/http/client.hpp>

class AddServerPopup;

class ServerTestPopup : public geode::Popup<const std::string&, AddServerPopup*> {
public:
    constexpr static float POPUP_WIDTH = 180.f;
    constexpr static float POPUP_HEIGHT = 80.f;

    ~ServerTestPopup();
    void onTimeout();

    static ServerTestPopup* create(const std::string&, AddServerPopup* parent);

protected:
    AddServerPopup* parent;
    std::optional<GHTTPRequestHandle> sentRequestHandle;

    bool setup(const std::string&, AddServerPopup* parent) override;

    void cancelRequest();
};