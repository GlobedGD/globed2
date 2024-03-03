#include "audio_setup_popup.hpp"

#ifdef GLOBED_VOICE_SUPPORT

#include "audio_device_cell.hpp"
#include <audio/manager.hpp>
#include <audio/voice_playback_manager.hpp>
#include <managers/settings.hpp>
#include <util/misc.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool AudioSetupPopup::setup() {
    auto menu = Build<CCMenu>::create()
        .pos(0.f, 0.f)
        .parent(m_mainLayer);

    auto winSize = CCDirector::get()->getWinSize();
    auto screenCenter = winSize / 2;

    Build<CCMenu>::create()
        .pos(screenCenter.width, screenCenter.height - 110.f)
        .layout(RowLayout::create()
                    ->setGap(5.0f)
                    ->setAxisReverse(true)
        )
        .parent(m_mainLayer)
        .id("audio-visualizer-menu"_spr)
        .store(visualizerLayout);

    // record button
    recordButton = Build<CCSprite>::createSpriteName("GJ_playBtn2_001.png")
        .scale(0.485f)
        .intoMenuItem([this](auto) {
            auto& vpm = VoicePlaybackManager::get();
            vpm.prepareStream(-1);

            auto& vm = GlobedAudioManager::get();
            vm.setRecordBufferCapacity(1);
            auto result = vm.startRecordingRaw([this, &vpm](const float* pcm, size_t samples) {
                // calculate the avg audio volume
                this->audioLevel = util::misc::calculatePcmVolume(pcm, samples);

                // play back the audio
                vpm.playRawDataStreamed(-1, pcm, samples);
            });

            if (result.isErr()) {
                log::warn("failed to start recording: {}", result.unwrapErr());
                Notification::create(result.unwrapErr(), NotificationIcon::Error)->show();
                return;
            }

            this->toggleButtons(true);
            this->audioVisualizer->resetMaxVolume();
        })
        .parent(visualizerLayout)
        .id("record-button"_spr)
        .collect();

    // stop recording button
    stopRecordButton = Build<CCSprite>::createSpriteName("GJ_stopEditorBtn_001.png")
        .intoMenuItem([this](auto) {
            this->toggleButtons(false);

            auto& vm = GlobedAudioManager::get();
            vm.haltRecording();

            auto& vpm = VoicePlaybackManager::get();
            vpm.removeStream(-1);
        })
        .parent(visualizerLayout)
        .id("stop-recording-button"_spr)
        .collect();

    // refresh list button
    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png")
        .intoMenuItem([this](auto) {
            this->refreshList();
        })
        .pos(screenCenter.width + POPUP_WIDTH / 2 - 10.f, screenCenter.height - POPUP_HEIGHT / 2 + 10.f)
        .parent(menu)
        .id("refresh-btn"_spr);

    Build<GlobedAudioVisualizer>::create()
        .parent(visualizerLayout)
        .id("audio-visualizer"_spr)
        .store(audioVisualizer);

    this->toggleButtons(false);

    listLayer = GJCommentListLayer::create(nullptr, "", util::ui::BG_COLOR_BROWN, LIST_WIDTH, LIST_HEIGHT, false);
    this->refreshList();

    float xpos = (m_mainLayer->getScaledContentSize().width - LIST_WIDTH) / 2;
    listLayer->setPosition({xpos, 85.f});
    m_mainLayer->addChild(listLayer);

    this->scheduleUpdate();

    return true;
}

void AudioSetupPopup::update(float) {
    audioVisualizer->setVolume(audioLevel);
}

cocos2d::CCArray* AudioSetupPopup::createDeviceCells() {
    auto cells = CCArray::create();

    auto& vm = GlobedAudioManager::get();

    int activeId = vm.getRecordingDevice().id;
    auto devices = vm.getRecordingDevices();

    for (const auto& device : devices) {
        cells->addObject(AudioDeviceCell::create(device, this, activeId));
    }

    return cells;
}

void AudioSetupPopup::refreshList() {
    if (listLayer->m_list)
        listLayer->m_list->removeFromParent();

    listLayer->m_list = ListView::create(createDeviceCells(), AudioDeviceCell::CELL_HEIGHT, LIST_WIDTH, LIST_HEIGHT);
    listLayer->addChild(listLayer->m_list);

    geode::cocos::handleTouchPriority(this);
}

void AudioSetupPopup::weakRefreshList() {
    auto& vm = GlobedAudioManager::get();
    auto recordDevices = vm.getRecordingDevices();
    size_t existingCount = listLayer->m_list->m_entries->count();
    if (existingCount != recordDevices.size()) {
        // if different device count, hard refresh
        this->refreshList();
        return;
    }

    int activeId = vm.getRecordingDevice().id;

    size_t refreshed = 0;
    for (auto* cell : CCArrayExt<AudioDeviceCell*>(listLayer->m_list->m_entries)) {
        for (auto& rdev : recordDevices) {
            if (rdev.id == cell->deviceInfo.id) {
                cell->refreshDevice(rdev, activeId);
                refreshed++;
            }
        }
    }

    // if the wrong amount of cells was refreshed, hard refresh
    if (refreshed != existingCount) {
        this->refreshList();
    }
}

void AudioSetupPopup::onClose(cocos2d::CCObject* sender) {
    Popup::onClose(sender);
    auto& vm = GlobedAudioManager::get();
    vm.haltRecording();
}

void AudioSetupPopup::toggleButtons(bool recording) {
    recordButton->removeFromParent();
    stopRecordButton->removeFromParent();

    if (recording) {
        visualizerLayout->addChild(stopRecordButton);
    } else {
        visualizerLayout->addChild(recordButton);
    }

    visualizerLayout->updateLayout();
}

void AudioSetupPopup::applyAudioDevice(int id) {
    auto& vm = GlobedAudioManager::get();
    if (vm.isRecording()) {
        Notification::create("Cannot switch device while recording", NotificationIcon::Error, 3.0f)->show();
        return;
    }

    GlobedAudioManager::get().setActiveRecordingDevice(id);
    auto& settings = GlobedSettings::get();
    settings.communication.audioDevice = id;
    settings.save();

    this->weakRefreshList();
}

AudioSetupPopup* AudioSetupPopup::create() {
    auto ret = new AudioSetupPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

#endif // GLOBED_VOICE_SUPPORT
