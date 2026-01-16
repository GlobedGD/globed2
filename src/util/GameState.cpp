#include <globed/util/GameState.hpp>

#include <Geode/modify/AppDelegate.hpp>
#include <Geode/modify/CCTouchDispatcher.hpp>
#include <asp/time.hpp>

using namespace geode::prelude;
using namespace asp::time;

static bool g_minimized = false;

namespace globed {

GameState getCurrentGameState()
{
    bool focused = isGameFocused();
    bool minimized = g_minimized;
    auto sinceInput = Duration::fromMillis(timeSinceInput());

    // log::debug("Game focus: {}, minimized: {}, since input: {}", focused, minimized, sinceInput.toString());

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

#ifndef GEODE_IS_DESKTOP
// Cannot unfocus on mobile platforms
bool isGameFocused()
{
    return true;
}
#endif

#ifndef GEODE_IS_WINDOWS

static Instant g_lastInputTime = Instant::now();

class $modify(CCTouchDispatcher) {
    $override void touches(CCSet *pTouches, CCEvent *pEvent, unsigned int uIndex)
    {
        if (pTouches && pTouches->count()) {
            g_lastInputTime = Instant::now();
        }

        CCTouchDispatcher::touches(pTouches, pEvent, uIndex);
    }
};

uint64_t timeSinceInput()
{
    return g_lastInputTime.elapsed().millis();
}

#endif

struct GLOBED_MODIFY_ATTR GameStateAppDelegateHook : public Modify<GameStateAppDelegateHook, AppDelegate> {
    $override void applicationDidEnterBackground()
    {
        g_minimized = true;
        AppDelegate::applicationDidEnterBackground();
    }

    $override void applicationWillEnterForeground()
    {
        g_minimized = false;
        AppDelegate::applicationWillEnterForeground();
    }
};

} // namespace globed