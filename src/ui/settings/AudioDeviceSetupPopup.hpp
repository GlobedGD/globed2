#pragma once

#include <globed/audio/VolumeEstimator.hpp>
#include <globed/audio/sound/VoiceStream.hpp>
#include <globed/prelude.hpp>
#include <ui/BasePopup.hpp>
#include <ui/misc/AudioVisualizer.hpp>

#include <asp/sync.hpp>
#include <cue/ListNode.hpp>

namespace globed {

class AudioDeviceSetupPopup : public BasePopup<AudioDeviceSetupPopup> {
public:
    static constexpr CCSize POPUP_SIZE{400.f, 260.f};
    static constexpr CCSize LIST_SIZE{340.f, 180.f};

    void applyAudioDevice(int id);

private:
    Ref<CCMenuItemSpriteExtra> m_recordButton, m_stopButton;
    cue::ListNode *m_list;
    AudioVisualizer *m_visualizer;
    CCMenu *m_visualizerContainer;
    VolumeEstimator m_estimator;
    std::shared_ptr<VoiceStream> m_stream;

    bool setup() override;
    void update(float) override;
    void onClose(cocos2d::CCObject *) override;
    void toggleButtons(bool recording);
    void refreshList();
    void weakRefreshList();
    void stopRecording();
};

} // namespace globed
