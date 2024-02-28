#pragma once
#include <defs/all.hpp>

#include <Geode/utils/web.hpp>

class AddServerPopup;

class ServerTestPopup : public geode::Popup<const std::string_view, AddServerPopup*> {
public:
    constexpr static float POPUP_WIDTH = 180.f;
    constexpr static float POPUP_HEIGHT = 80.f;

    ~ServerTestPopup();
    void onTimeout();

    static ServerTestPopup* create(const std::string_view, AddServerPopup* parent);

protected:
    AddServerPopup* parent;
    std::optional<geode::utils::web::SentAsyncWebRequestHandle> sentRequestHandle;

    bool setup(const std::string_view, AddServerPopup* parent) override;

    void cancelRequest();
};