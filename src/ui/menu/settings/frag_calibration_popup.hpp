#pragma once
#include <defs/all.hpp>

#include <data/packets/server/connection.hpp>
#include <util/time.hpp>

class FragmentationCalibartionPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 180.f;
    static constexpr float POPUP_HEIGHT = 80.f;
    static FragmentationCalibartionPopup* create();
    static constexpr size_t TEST_PACKET_SIZES[] = {
        1000, 1300, 1400, 1450, 2000, 4000, 7000, 10000, 13000, 18000, 25000, 30000, 40000, 50000, 60000, 65000, 0
    };

private:
    cocos2d::CCLabelBMFont* statusLabel = nullptr;
    size_t currentSize = 0;
    size_t currentAttempt = 0;
    size_t currentSizeIdx = 0;
    int uid;
    int failedAttempts = 0;
    util::time::system_time_point lastPacket;

    bool setup() override;
    void onClose(cocos2d::CCObject*) override;
    void checkForUpdates(float);

    void nextStep();
    void closeDelayed();
};
