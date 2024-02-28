#pragma once
#include <defs/all.hpp>

#if GLOBED_VOICE_SUPPORT

#include <audio/manager.hpp>

class AudioSetupPopup;

class AudioDeviceCell : public cocos2d::CCLayer {
public:
    static constexpr float CELL_HEIGHT = 40.f;

    static AudioDeviceCell* create(const AudioRecordingDevice& device, AudioSetupPopup* parent, int activeId);

    void refreshDevice(const AudioRecordingDevice& device, int activeId);

private:
    AudioRecordingDevice deviceInfo;
    AudioSetupPopup* parent;
    CCMenuItemSpriteExtra* btnSelect;
    cocos2d::CCSprite* btnSelected;
    cocos2d::CCLabelBMFont* labelName;

    friend class AudioSetupPopup;

    bool init(const AudioRecordingDevice& device, AudioSetupPopup* parent, int activeId);
};

#endif // GLOBED_VOICE_SUPPORT