#include "AudioDeviceSetupPopup.hpp"

#include <UIBuilder.hpp>
#include <globed/audio/AudioManager.hpp>
#include <asp/iter.hpp>

using namespace geode::prelude;

namespace globed {

static constexpr float CELL_HEIGHT = 32.f;

class AudioDeviceCell : public CCMenu {
public:
    static AudioDeviceCell* create(const AudioRecordingDevice& device, AudioDeviceSetupPopup* popup, int activeId) {
        auto cell = new AudioDeviceCell();
        cell->m_device = device;
        cell->m_popup = popup;
        cell->m_activeId = activeId;
        cell->init();
        cell->autorelease();
        return cell;
    }

private:
    friend class AudioDeviceSetupPopup;
    AudioRecordingDevice m_device;
    AudioDeviceSetupPopup* m_popup;
    CCMenuItemSpriteExtra* m_btnSelect;
    CCSprite* m_btnSelected;
    CCLabelBMFont* m_labelName;
    int m_activeId;

    bool init() {
        CCMenu::init();

        float width = AudioDeviceSetupPopup::LIST_SIZE.width;

        this->ignoreAnchorPointForPosition(false);
        this->setContentSize({width, CELL_HEIGHT});

        CCPoint btnPos { AudioDeviceSetupPopup::LIST_SIZE.width - 24.f, CELL_HEIGHT / 2.f };

        // select device
        m_btnSelect = Build<CCSprite>::createSpriteName("GJ_selectSongBtn_001.png")
            .scale(0.7f)
            .intoMenuItem([this](auto) {
                m_popup->applyAudioDevice(m_device.id);
            })
            .visible(m_device.id != m_activeId)
            .id("select-device-btn"_spr)
            .pos(btnPos)
            .parent(this);

        // selected device
        m_btnSelected = Build<CCSprite>::createSpriteName("GJ_selectSongOnBtn_001.png")
            .scale(0.7f)
            .visible(m_device.id == m_activeId)
            .id("selected-device-btn"_spr)
            .pos(btnPos)
            .parent(this);

        // device name
        m_labelName = Build<CCLabelBMFont>::create(m_device.name.c_str(), "goldFont.fnt")
            .limitLabelWidth(280.f, 0.7f, 0.1f)
            .anchorPoint(0.f, 0.5f)
            .pos(5.f, CELL_HEIGHT / 2)
            .parent(this)
            .id("device-name-label"_spr);

        return true;
    }

    void refresh(const AudioRecordingDevice& device, int activeId) {
        m_device = device;
        m_activeId = activeId;

        m_labelName->setString(m_device.name.c_str());
        m_btnSelect->setVisible(m_device.id != m_activeId);
        m_btnSelected->setVisible(m_device.id == m_activeId);
    }
};

bool AudioDeviceSetupPopup::setup() {
    this->setID("AudioSetupPopup"_spr);

    auto menu = Build<CCMenu>::create()
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    m_visualizerContainer = Build<CCMenu>::create()
        .pos(this->fromBottom(32.f))
        .layout(RowLayout::create()
            ->setGap(5.0f)
            ->setAxisReverse(true)
        )
        .parent(m_mainLayer)
        .id("audio-visualizer-menu"_spr);

    // record button
    m_recordButton = Build<CCSprite>::createSpriteName("GJ_playBtn2_001.png")
        .scale(0.485f)
        .intoMenuItem([this] {
            auto& am = AudioManager::get();

            am.setActiveRecordingDevice(globed::setting<int>("core.audio.input-device"));
            am.setRecordBufferCapacity(1);
            auto result = am.startRecordingRaw([this](const float* pcm, size_t samples) {
                // play back the audio
                AudioManager::get().playFrameStreamedRaw(-1, pcm, samples);
            });

            if (result.isErr()) {
                log::warn("failed to start recording: {}", result.unwrapErr());
                Notification::create(result.unwrapErr(), NotificationIcon::Error)->show();
                return;
            }

            this->toggleButtons(true);
            m_visualizer->resetMaxVolume();
        })
        .parent(m_visualizerContainer)
        .id("record-button"_spr)
        .collect();

    // stop recording button
    m_stopButton = Build<CCSprite>::createSpriteName("GJ_stopEditorBtn_001.png")
        .intoMenuItem([this](auto) {
            this->toggleButtons(false);

            auto& am = AudioManager::get();
            am.haltRecording();
            am.stopAllOutputStreams();
        })
        .parent(m_visualizerContainer)
        .id("stop-recording-button"_spr)
        .collect();

    // refresh list button
    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png")
        .intoMenuItem([this](auto) {
            this->refreshList();
        })
        .pos(this->fromBottomRight(5.f, 5.f))
        .parent(menu)
        .id("refresh-btn"_spr);

    m_visualizer = Build<AudioVisualizer>::create()
        .parent(m_visualizerContainer)
        .id("audio-visualizer"_spr);

    this->toggleButtons(false);

    m_list = Build(cue::ListNode::create(LIST_SIZE, cue::Brown, cue::ListBorderStyle::Comments))
        .anchorPoint(0.5f, 1.f)
        .pos(this->fromTop(20.f))
        .parent(m_mainLayer);

    this->refreshList();

    this->scheduleUpdate();

    return true;
}

void AudioDeviceSetupPopup::update(float dt) {
    auto stream = AudioManager::get().getStream(-1);
    if (stream) {
        stream->updateEstimator(dt);
        m_visualizer->setVolume(stream->getLoudness());
    }
}

void AudioDeviceSetupPopup::refreshList() {
    auto& am = AudioManager::get();

    int activeId = -1;
    if (const auto& dev = am.getRecordingDevice()) {
        activeId = dev->id;
    }

    auto devices = am.getRecordingDevices();
    for (const auto& device : devices) {
        m_list->addCell(AudioDeviceCell::create(device, this, activeId));
    }

    geode::cocos::handleTouchPriority(this);
}

void AudioDeviceSetupPopup::weakRefreshList() {
    int active = globed::setting<int>("core.audio.input-device");
    auto& am = AudioManager::get();
    auto recordDevices = am.getRecordingDevices();

    if (m_list->size() != recordDevices.size()) {
        // if different device count, hard refresh
        this->refreshList();
        return;
    }

    int activeId = -1;
    if (const auto& dev = am.getRecordingDevice()) {
        activeId = dev->id;
    }

    size_t refreshed = 0;
    for (auto* cell : m_list->iter<AudioDeviceCell>()) {
        for (auto& rdev : recordDevices) {
            if (rdev.id == cell->m_device.id) {
                cell->refresh(rdev, activeId);
                refreshed++;
                break;
            }
        }
    }

    // if the wrong amount of cells was refreshed, hard refresh
    if (refreshed != recordDevices.size()) {
        this->refreshList();
    }
}

void AudioDeviceSetupPopup::onClose(cocos2d::CCObject* sender) {
    Popup::onClose(sender);
    auto& am = AudioManager::get();
    am.haltRecording();
}

void AudioDeviceSetupPopup::toggleButtons(bool recording) {
    m_recordButton->removeFromParent();
    m_stopButton->removeFromParent();

    if (recording) {
        m_visualizerContainer->addChild(m_stopButton);
    } else {
        m_visualizerContainer->addChild(m_recordButton);
    }

    m_visualizerContainer->updateLayout();
}

void AudioDeviceSetupPopup::applyAudioDevice(int id) {
    auto& vm = AudioManager::get();
    if (vm.isRecording()) {
        Notification::create("Cannot switch device while recording", NotificationIcon::Error, 3.0f)->show();
        return;
    }

    globed::setting<int>("core.audio.input-device") = id;
    vm.setActiveRecordingDevice(id);

    this->weakRefreshList();
}

}
