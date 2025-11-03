#include <globed/util/GameState.hpp>

#include <Geode/modify/CCTouchDispatcher.hpp>
#include <Geode/modify/AppDelegate.hpp>
#include <asp/time.hpp>

#ifdef GEODE_IS_WINDOWS
# include <Windows.h>
#endif

using namespace geode::prelude;
using namespace asp::time;

static bool g_minimized = false;

namespace globed {

static bool isGameFocused();
static Duration timeSinceInput();

GameState getCurrentGameState() {
    bool focused = isGameFocused();
    bool minimized = g_minimized;
    auto sinceInput = timeSinceInput();

    if (focused && sinceInput < Duration::fromSecs(60)) {
        return GameState::Active;
    } else if (focused) {
        return GameState::Afk;
    } else if (minimized) {
        return GameState::Closed;
    } else if (sinceInput < Duration::fromSecs(180)) {
        return GameState::Afk;
    } else {
        return GameState::Closed;
    }
}

#ifdef GEODE_IS_WINDOWS

bool isGameFocused() {
    HWND wnd = WindowFromDC(wglGetCurrentDC());
    return GetForegroundWindow() == wnd;
}

Duration timeSinceInput() {
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    GetLastInputInfo(&lii);
    DWORD tickCount = GetTickCount();
    DWORD elapsed = tickCount - lii.dwTime;
    return Duration::fromMillis(elapsed);
}

#else

bool isGameFocused() {
    return true;
}

static Instant g_lastInputTime = Instant::now();

class $modify(CCTouchDispatcher) {
    $override
    void touches(CCSet *pTouches, CCEvent *pEvent, unsigned int uIndex) {
        if (pTouches && pTouches->count()) {
            g_lastInputTime = Instant::now();
        }

        CCTouchDispatcher::touches(pTouches, pEvent, uIndex);
    }
};

Duration timeSinceInput() {
    return g_lastInputTime.elapsed();
}

#endif

struct GLOBED_MODIFY_ATTR GameStateAppDelegateHook : public Modify<GameStateAppDelegateHook, AppDelegate> {
    $override
    void applicationDidEnterBackground() {
        g_minimized = true;
        AppDelegate::applicationDidEnterBackground();
    }

    $override
    void applicationWillEnterForeground() {
        g_minimized = false;
        AppDelegate::applicationWillEnterForeground();
    }
};

}