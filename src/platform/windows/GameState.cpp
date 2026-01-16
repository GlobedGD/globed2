#include <Windows.h>
#include <globed/util/GameState.hpp>

using namespace geode::prelude;

namespace globed {

bool isGameFocused()
{
    HWND wnd = WindowFromDC(wglGetCurrentDC());
    return GetForegroundWindow() == wnd;
}

uint64_t timeSinceInput()
{
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    GetLastInputInfo(&lii);
    DWORD tickCount = GetTickCount();
    DWORD elapsed = tickCount - lii.dwTime;
    return elapsed;
}

} // namespace globed