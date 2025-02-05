#pragma once

#include <util/singleton.hpp>
#include <string>
#include <map>

#include <Geode/cocos/layers_scenes_transitions_nodes/CCLayer.h>
#include <managers/settings.hpp>

// Big shoutout to eclipse menu i am too lazy to type all that out myself
// https://github.com/EclipseMenu/EclipseMenu/blob/main/src/modules/keybinds/manager.hpp

namespace globed {
    enum class Key {
        None,

        // Letters
        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        // Symbols
        Space, Apostrophe, Comma, Minus, Period, Slash, Semicolon, Equal,
        LeftBracket, Backslash, RightBracket, Backtick,

        // Numbers
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

        // Function keys
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13,
        F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25,

        // Keypad
        NumPad0, NumPad1, NumPad2, NumPad3, NumPad4, NumPad5, NumPad6, NumPad7, NumPad8, NumPad9,
        NumPadDecimal, NumPadDivide, NumPadMultiply, NumPadSubtract, NumPadAdd, NumPadEnter, NumPadEqual,

        // Special keys
        Escape, Enter, Tab, Backspace, Insert, Delete, Home, End, PageUp, PageDown,
        CapsLock, ScrollLock, NumLock, PrintScreen, Pause,

        // Arrow keys
        Up, Down, Left, Right,
    };

    enum class Keybinds {
        VoiceChatKey,
        VoiceDeafenKey,
    };

    std::string formatKey(Key key);
    std::string formatKey(cocos2d::enumKeyCodes key);
}

class KeybindsManager : public SingletonBase<KeybindsManager> {
    friend class SingletonBase;

public:
    void handlePress(cocos2d::enumKeyCodes, std::function<void(cocos2d::enumKeyCodes)> callback);
    void handleRelease(globed::Key, std::function<void(globed::Key)> callback);
    bool isHeld(globed::Key key);
    static globed::Key convertCocosKey(cocos2d::enumKeyCodes key);

    cocos2d::enumKeyCodes voiceChatKey;
    cocos2d::enumKeyCodes voiceDeafenKey;

private:
    std::map<globed::Key, bool> heldKeys;
};