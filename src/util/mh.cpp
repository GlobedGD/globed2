#include <globed/util/mh.hpp>

#ifdef GEODE_IS_WINDOWS
# include <Windows.h>
#elif defined(GEODE_IS_ANDROID)
# include <dlfcn.h>
#endif

using namespace geode::prelude;

using SetHackEnabledFn = void(*)(const std::string& hack, bool enabled);
using GetHackEnabledFn = bool(*)(const std::string& hack);

#ifdef GEODE_IS_WINDOWS

static HMODULE getMegahack() {
    return GetModuleHandleW(L"absolllute.megahack.dll");
}

namespace globed::mh {

bool isLoaded() {
    return getMegahack();
}

bool isHackEnabled(const std::string& hack) {
    auto mod = getMegahack();
    if (!mod) return false;

    auto symbol = GetProcAddress(mod, "?isHackEnabled@extensions@hackpro@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z");
    if (!symbol) return false;

    return reinterpret_cast<GetHackEnabledFn>(symbol)(hack);
}

void setHackEnabled(const std::string& hack, bool enabled) {
    auto mod = getMegahack();
    if (!mod) return;

    auto symbol = GetProcAddress(mod, "?setHackEnabled@extensions@hackpro@@YAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@_N@Z");
    if (!symbol) return;

    reinterpret_cast<SetHackEnabledFn>(symbol)(hack, enabled);
}

}

#elif defined (GEODE_IS_ANDROID)

static auto getMegahack() {
    auto handle = dlopen("absolllute.megahack.so", RTLD_LAZY | RTLD_NOLOAD);
    return handle;
}

namespace globed::mh {

bool isLoaded() {
    return getMegahack();
}

bool isHackEnabled(const std::string& hack) {
    auto mod = getMegahack();
    if (!mod) return false;

    // TODO

    return false;
}

void setHackEnabled(const std::string& hack, bool enabled) {
    auto mod = getMegahack();
    if (!mod) return;

    // TODO
}

}

#else

namespace globed::mh {

bool isLoaded() { return false; }
bool isHackEnabled(const std::string& hack) { return false; }
void setHackEnabled(const std::string& hack, bool enabled) {}

}

#endif