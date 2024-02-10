#pragma once
#include <defs.hpp>

#include <data/packets/server/connection.hpp>
#include <util/time.hpp>

class ConnectionTestPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 180.f;
    static constexpr float POPUP_HEIGHT = 80.f;
    static ConnectionTestPopup* create();

private:
    cocos2d::CCLabelBMFont* statusLabel = nullptr;
    size_t currentSize = 0;
    size_t currentAttempt = 0;
    int uid;
    int failedAttempts = 0;
    util::time::system_time_point lastPacket;

    bool setup() override;
    void onClose(cocos2d::CCObject*) override;
    void checkForUpdates(float);

    void nextStep();
    void closeDelayed();
};
