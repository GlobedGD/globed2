#include "Geode/cocos/robtop/keyboard_dispatcher/CCKeyboardDelegate.h"
#include "Geode/ui/Notification.hpp"
#include <audio/voice_playback_manager.hpp>
#include <audio/manager.hpp>
#include <defs/geode.hpp>
#include <defs/platform.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <managers/keybinds.hpp>

#include <Geode/modify/UILayer.hpp>

using namespace geode::prelude;

struct GLOBED_DLL HookedUILayer : geode::Modify<HookedUILayer, UILayer> {
    static void onModify(auto& self) {
        (void) self.setHookPriority("UILayer::handleKeypress", 9999);
    }

    void handleKeypress(enumKeyCodes p0, bool p1) {
        auto gjbgl = GlobedGJBGL::get();
        if (gjbgl) gjbgl->m_fields->isManuallyResettingLevel = true;
        UILayer::handleKeypress(p0, p1);
        if (gjbgl) gjbgl->m_fields->isManuallyResettingLevel = false;        
    }

    void keyDown(enumKeyCodes p0) {
        auto& settings = GlobedSettings::get();
        auto& fields = GlobedGJBGL::get()->getFields();

#ifdef GLOBED_VOICE_CAN_TALK
        if (p0 == (cocos2d::enumKeyCodes)settings.communication.voiceChatKey.get()) {
            if (!fields.deafened) {
                GlobedAudioManager::get().resumePassiveRecording();
            }
        } else if (p0 == (cocos2d::enumKeyCodes)settings.communication.voiceDeafenKey.get()) {
            auto& vpm = VoicePlaybackManager::get();
            
            fields.deafened = !fields.deafened;
            if (fields.deafened) {
                vpm.muteEveryone();
                GlobedAudioManager::get().pausePassiveRecording();
                if (settings.communication.deafenNotification)
                    Notification::create("Deafened Voice Chat", CCSprite::createWithSpriteFrameName("deafen-icon-on.png"_spr), 0.2f)->show();
            } else {
                if (settings.communication.deafenNotification)
                    Notification::create("Undeafened Voice Chat", CCSprite::createWithSpriteFrameName("deafen-icon-off.png"_spr), 0.2f)->show();
                if (!fields.isVoiceProximity)
                    vpm.setVolumeAll(settings.communication.voiceVolume);
            }
        }
#endif
        if (p0 == (cocos2d::enumKeyCodes)settings.globed.hidePlayersKey.get()) {
            bool newState = !fields.arePlayersHidden;
            GlobedGJBGL::get()->setPlayerVisibility(newState);
            Notification::create((newState) ? "All Players Hidden" : "All Players Visible", NotificationIcon::Success, 0.2f)->show();
        }

        UILayer::keyDown(p0);
    }

    void keyUp(enumKeyCodes p0) {
        auto& settings = GlobedSettings::get();

#ifdef GLOBED_VOICE_CAN_TALK
        if (p0 == (cocos2d::enumKeyCodes)settings.communication.voiceChatKey.get()) {
            GlobedAudioManager::get().pausePassiveRecording();
        }
#endif

        UILayer::keyUp(p0);
    }
};
