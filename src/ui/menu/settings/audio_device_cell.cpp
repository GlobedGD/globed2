#include "audio_device_cell.hpp"

#ifdef GLOBED_VOICE_SUPPORT

#include "audio_setup_popup.hpp"

using namespace geode::prelude;

bool AudioDeviceCell::init(const AudioRecordingDevice& device, AudioSetupPopup* parent, int activeId) {
    if (!CCLayer::init()) return false;
    this->deviceInfo = device;
    this->parent = parent;

    // select device
    Build<CCSprite>::createSpriteName("GJ_selectSongBtn_001.png")
        .scale(0.8f)
        .intoMenuItem([this](auto) {
            this->parent->applyAudioDevice(this->deviceInfo.id);
        })
        .visible(device.id != activeId)
        .id("select-device-btn"_spr)
        .pos(AudioSetupPopup::LIST_WIDTH - 5.f, CELL_HEIGHT / 2)
        .store(btnSelect)
        .intoNewParent(CCMenu::create())
        .pos(0.f, 0.f)
        .parent(this);

    // selected device
    Build<CCSprite>::createSpriteName("GJ_selectSongOnBtn_001.png")
        .scale(0.8f)
        .visible(device.id == activeId)
        .pos(AudioSetupPopup::LIST_WIDTH - 5.f, CELL_HEIGHT / 2)
        .parent(this)
        .id("selected-device-btn"_spr)
        .store(btnSelected);

    btnSelect->setPositionX(btnSelect->getPositionX() - btnSelect->getScaledContentSize().width / 2);
    btnSelected->setPositionX(btnSelected->getPositionX() - btnSelected->getScaledContentSize().width / 2);

    // device name
    Build<CCLabelBMFont>::create(device.name.c_str(), "goldFont.fnt")
        .limitLabelWidth(290.f, 0.8f, 0.1f)
        .anchorPoint(0.f, 0.5f)
        .pos(5.f, CELL_HEIGHT / 2)
        .parent(this)
        .id("device-name-label"_spr)
        .store(labelName);

    return true;
}

void AudioDeviceCell::refreshDevice(const AudioRecordingDevice& device, int activeId) {
    this->deviceInfo = device;
    btnSelect->setVisible(device.id != activeId);
    btnSelected->setVisible(device.id == activeId);
    labelName->setString(device.name.c_str());
}

AudioDeviceCell* AudioDeviceCell::create(const AudioRecordingDevice& device, AudioSetupPopup* parent, int activeId) {
    auto ret = new AudioDeviceCell;
    if (ret->init(device, parent, activeId)) {
        return ret;
    }

    delete ret;
    return nullptr;
}

#endif // GLOBED_VOICE_SUPPORT