#pragma once
#include <defs/all.hpp>

#if GLOBED_VOICE_SUPPORT

#include <ui/general/audio_visualizer.hpp>
#include <util/sync.hpp>

class AudioSetupPopup : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 400.f;
    static constexpr float POPUP_HEIGHT = 280.f;
    static constexpr float LIST_WIDTH = 340.f;
    static constexpr float LIST_HEIGHT = 200.f;

    void applyAudioDevice(int id);

    static AudioSetupPopup* create();

private:
    Ref<CCMenuItemSpriteExtra> recordButton, stopRecordButton;
    GJCommentListLayer* listLayer;
    GlobedAudioVisualizer* audioVisualizer;
    util::sync::AtomicF32 audioLevel;
    cocos2d::CCMenu* visualizerLayout;

    bool setup() override;
    void update(float) override;
    void refreshList();
    void weakRefreshList();
    void onClose(cocos2d::CCObject*) override;
    void toggleButtons(bool recording);

    cocos2d::CCArray* createDeviceCells();
};

#endif // GLOBED_VOICE_SUPPORT
