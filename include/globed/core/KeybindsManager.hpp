#pragma once
#include <globed/util/singleton.hpp>

namespace globed {

// Big shoutout to eclipse menu i am too lazy to type all that out myself
// https://github.com/EclipseMenu/EclipseMenu/blob/main/src/modules/keybinds/manager.hpp

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

const char* formatKey(Key key);
const char* formatKey(cocos2d::enumKeyCodes key);
Key convertCocosKey(cocos2d::enumKeyCodes key);

class GLOBED_DLL KeybindsManager : public SingletonBase<KeybindsManager> {
public:
    void handleKeyDown(cocos2d::enumKeyCodes key);
    void handleKeyUp(cocos2d::enumKeyCodes key);
    void refreshBinds();
    void releaseAll();
    bool isKeyUsed(cocos2d::enumKeyCodes key);
    bool isAnyEmoteKeyBound();

protected:
    cocos2d::enumKeyCodes m_kVoice, m_kDeafen, m_kHidePlayers;
    std::array<cocos2d::enumKeyCodes, 8> m_emoteBinds;

    friend class SingletonBase;
    KeybindsManager();

    cocos2d::enumKeyCodes getBind(std::string_view key);
};

}