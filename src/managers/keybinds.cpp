#include "keybinds.hpp"

#include <defs/geode.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <audio/manager.hpp>
#include <audio/voice_playback_manager.hpp>
#include <util/into.hpp>
#include <util/misc.hpp>

using namespace geode::prelude;
using globed::Key;

void KeybindsManager::handleKeyDown(cocos2d::enumKeyCodes key) {
    auto gjbgl = GLOBED_LAZY(GlobedGJBGL::get());
    auto fields = GLOBED_LAZY(&gjbgl->getFields());

#ifdef GLOBED_VOICE_CAN_TALK
    if (key == keyVoice) {
        if (!fields->deafened) {
            GlobedAudioManager::get().resumePassiveRecording();
        }
    }
    else if (key == keyDeafen) {
        auto& vpm = VoicePlaybackManager::get();
        auto& settings = GlobedSettings::get();

        fields->deafened = !fields->deafened;
        if (fields->deafened) {
            vpm.muteEveryone();
            GlobedAudioManager::get().pausePassiveRecording();
            if (settings.communication.deafenNotification) {
                Notification::create("Deafened Voice Chat", CCSprite::createWithSpriteFrameName("deafen-icon-on.png"_spr), 0.2f)->show();
            }
        } else {
            if (settings.communication.deafenNotification) {
                Notification::create("Undeafened Voice Chat", CCSprite::createWithSpriteFrameName("deafen-icon-off.png"_spr), 0.2f)->show();
            }

            if (!fields->isVoiceProximity) {
                vpm.setVolumeAll(settings.communication.voiceVolume);
            }
        }
    }
    else
#endif
    if (key == keyHidePlayers) {
        bool newState = !fields->arePlayersHidden;
        gjbgl->setPlayerVisibility(newState);
        Notification::create((newState) ? "All Players Hidden" : "All Players Visible", NotificationIcon::Success, 0.2f)->show();
    }
}

void KeybindsManager::handleKeyUp(cocos2d::enumKeyCodes key) {
#ifdef GLOBED_VOICE_CAN_TALK
    if (key == keyVoice) {
        GlobedAudioManager::get().pausePassiveRecording();
    }
    else if (key == keyDeafen) {}
    else
#endif
    if (key == keyHidePlayers) {}
}

void KeybindsManager::refreshBinds() {
    auto& keys = GlobedSettings::get().keys;

    keyVoice = keys.voiceChatKey.get();
    keyDeafen = keys.voiceDeafenKey.get();
    keyHidePlayers = keys.hidePlayersKey.get();
}

bool KeybindsManager::isKeyUsed(cocos2d::enumKeyCodes key) {
    return key == keyVoice
        || key == keyDeafen
        || key == keyHidePlayers;
}

namespace globed {
    const char* formatKey(Key key) {
#define KCASE(k) case Key::k: return #k
#define KCASEN(k, n) case Key::k: return n

        switch (key) {
            KCASE(A); KCASE(B); KCASE(C); KCASE(D); KCASE(E); KCASE(F); KCASE(G); KCASE(H); KCASE(I);
            KCASE(J); KCASE(K); KCASE(L); KCASE(M); KCASE(N); KCASE(O); KCASE(P); KCASE(Q); KCASE(R);
            KCASE(S); KCASE(T); KCASE(U); KCASE(V); KCASE(W); KCASE(X); KCASE(Y); KCASE(Z);

            KCASE(Space); KCASEN(Apostrophe, "'"); KCASEN(Comma, ","); KCASEN(Minus, "-"); KCASEN(Period, "."); KCASEN(Slash, "/"); KCASEN(Semicolon, ";"); KCASEN(Equal, "=");
            KCASEN(LeftBracket, "{"); KCASEN(RightBracket, "}"); KCASEN(Backslash, "\\"); KCASEN(Backtick, "`");

            KCASEN(Num0, "0"); KCASEN(Num1, "1"); KCASEN(Num2, "2"); KCASEN(Num3, "3"); KCASEN(Num4, "4");
            KCASEN(Num5, "5"); KCASEN(Num6, "6"); KCASEN(Num7, "7"); KCASEN(Num8, "8"); KCASEN(Num9, "9");

            KCASE(F1); KCASE(F2); KCASE(F3); KCASE(F4); KCASE(F5); KCASE(F6); KCASE(F7); KCASE(F8); KCASE(F9); KCASE(F10); KCASE(F11); KCASE(F12);
            KCASE(F13); KCASE(F14); KCASE(F15); KCASE(F16); KCASE(F17); KCASE(F18); KCASE(F19); KCASE(F20); KCASE(F21); KCASE(F22); KCASE(F23); KCASE(F24); KCASE(F25);

            KCASEN(NumPad0, "Num 0"); KCASEN(NumPad1, "Num 1"); KCASEN(NumPad2, "Num 2"); KCASEN(NumPad3, "Num 3"); KCASEN(NumPad4, "Num 4");
            KCASEN(NumPad5, "Num 5"); KCASEN(NumPad6, "Num 6"); KCASEN(NumPad7, "Num 7"); KCASEN(NumPad8, "Num 8"); KCASEN(NumPad9, "Num 9");
            KCASEN(NumPadDecimal, "Num ."); KCASEN(NumPadDivide, "Num /"); KCASEN(NumPadMultiply, "Num *");
            KCASEN(NumPadSubtract, "Num -"); KCASEN(NumPadAdd, "Num +"); KCASEN(NumPadEnter, "Num Enter"); KCASEN(NumPadEqual, "Num =");

            KCASE(Escape); KCASE(Enter); KCASE(Tab); KCASE(Backspace); KCASE(Insert); KCASE(Delete); KCASE(Home); KCASE(End);
            KCASEN(PageUp, "Page Up"); KCASEN(PageDown, "Page Down"); KCASEN(CapsLock, "Caps Lock"); KCASEN(ScrollLock, "Scroll Lock");
            KCASEN(NumLock, "Num Lock"); KCASEN(PrintScreen, "Print Screen"); KCASEN(Pause, "Pause");

            KCASE(Up); KCASE(Down); KCASE(Left); KCASE(Right);

            case Key::None: return "None";
        }
#undef KCASE
#undef KCASEN
    }

    const char* formatKey(enumKeyCodes key) {
        return formatKey(KeybindsManager::convertCocosKey(key));
    }
}

Key KeybindsManager::convertCocosKey(enumKeyCodes key) {
    switch (key) {
        case KEY_A: return Key::A;
        case KEY_B: return Key::B;
        case KEY_C: return Key::C;
        case KEY_D: return Key::D;
        case KEY_E: return Key::E;
        case KEY_F: return Key::F;
        case KEY_G: return Key::G;
        case KEY_H: return Key::H;
        case KEY_I: return Key::I;
        case KEY_J: return Key::J;
        case KEY_K: return Key::K;
        case KEY_L: return Key::L;
        case KEY_M: return Key::M;
        case KEY_N: return Key::N;
        case KEY_O: return Key::O;
        case KEY_P: return Key::P;
        case KEY_Q: return Key::Q;
        case KEY_R: return Key::R;
        case KEY_S: return Key::S;
        case KEY_T: return Key::T;
        case KEY_U: return Key::U;
        case KEY_V: return Key::V;
        case KEY_W: return Key::W;
        case KEY_X: return Key::X;
        case KEY_Y: return Key::Y;
        case KEY_Z: return Key::Z;
        case KEY_Space: return Key::Space;
        case KEY_Apostrophe: return Key::Apostrophe;
        case KEY_OEMComma: return Key::Comma;
        case KEY_OEMMinus: return Key::Minus;
        case KEY_OEMPeriod: return Key::Period;
        case KEY_Slash: return Key::Slash;
        case KEY_Semicolon: return Key::Semicolon;
        case KEY_Equal: return Key::Equal;
        case KEY_OEMEqual: return Key::Equal;
        case KEY_LeftBracket: return Key::LeftBracket;
        case KEY_Backslash: return Key::Backslash;
        case KEY_RightBracket: return Key::RightBracket;
        case KEY_GraveAccent: return Key::Backtick;
        case KEY_Zero: return Key::Num0;
        case KEY_One: return Key::Num1;
        case KEY_Two: return Key::Num2;
        case KEY_Three: return Key::Num3;
        case KEY_Four: return Key::Num4;
        case KEY_Five: return Key::Num5;
        case KEY_Six: return Key::Num6;
        case KEY_Seven: return Key::Num7;
        case KEY_Eight: return Key::Num8;
        case KEY_Nine: return Key::Num9;
        case KEY_F1: return Key::F1;
        case KEY_F2: return Key::F2;
        case KEY_F3: return Key::F3;
        case KEY_F4: return Key::F4;
        case KEY_F5: return Key::F5;
        case KEY_F6: return Key::F6;
        case KEY_F7: return Key::F7;
        case KEY_F8: return Key::F8;
        case KEY_F9: return Key::F9;
        case KEY_F10: return Key::F10;
        case KEY_F11: return Key::F11;
        case KEY_F12: return Key::F12;
        case KEY_F13: return Key::F13;
        case KEY_F14: return Key::F14;
        case KEY_F15: return Key::F15;
        case KEY_F16: return Key::F16;
        case KEY_F17: return Key::F17;
        case KEY_F18: return Key::F18;
        case KEY_F19: return Key::F19;
        case KEY_F20: return Key::F20;
        case KEY_F21: return Key::F21;
        case KEY_F22: return Key::F22;
        case KEY_F23: return Key::F23;
        case KEY_F24: return Key::F24;
        case KEY_NumPad0: return Key::NumPad0;
        case KEY_NumPad1: return Key::NumPad1;
        case KEY_NumPad2: return Key::NumPad2;
        case KEY_NumPad3: return Key::NumPad3;
        case KEY_NumPad4: return Key::NumPad4;
        case KEY_NumPad5: return Key::NumPad5;
        case KEY_NumPad6: return Key::NumPad6;
        case KEY_NumPad7: return Key::NumPad7;
        case KEY_NumPad8: return Key::NumPad8;
        case KEY_NumPad9: return Key::NumPad9;
        case KEY_NumEnter: return Key::NumPadEnter;
        case KEY_Decimal: return Key::NumPadDecimal;
        case KEY_Divide: return Key::NumPadDivide;
        case KEY_Multiply: return Key::NumPadMultiply;
        case KEY_Subtract: return Key::NumPadSubtract;
        case KEY_Add: return Key::NumPadAdd;
        case KEY_Escape: return Key::Escape;
        case KEY_Enter: return Key::Enter;
        case KEY_Tab: return Key::Tab;
        case KEY_Backspace: return Key::Backspace;
        case KEY_Insert: return Key::Insert;
        case KEY_Delete: return Key::Delete;
        case KEY_Home: return Key::Home;
        case KEY_End: return Key::End;
        case KEY_PageUp: return Key::PageUp;
        case KEY_PageDown: return Key::PageDown;
        case KEY_CapsLock: return Key::CapsLock;
        case KEY_ScrollLock: return Key::ScrollLock;
        case KEY_Numlock: return Key::NumLock;
        case KEY_PrintScreen: return Key::PrintScreen;
        case KEY_Pause: return Key::Pause;
        case KEY_ArrowUp: return Key::Up;
        case KEY_ArrowDown: return Key::Down;
        case KEY_ArrowLeft: return Key::Left;
        case KEY_ArrowRight: return Key::Right;
        default: return Key::None;
    }
}