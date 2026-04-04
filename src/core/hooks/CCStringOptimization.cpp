// Copied from blaze

#include <Geode/Geode.hpp>
#include <Geode/modify/CCString.hpp>
#include <globed/prelude.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR CCStringHook : Modify<CCStringHook, CCString> {
    static void onModify(auto& self) {
        (void) self.setHookPriority("cocos2d::CCString::floatValue", Priority::Replace);
        (void) self.setHookPriority("cocos2d::CCString::doubleValue", Priority::Replace);
        (void) self.setHookPriority("cocos2d::CCString::intValue", Priority::Replace);
        (void) self.setHookPriority("cocos2d::CCString::uintValue", Priority::Replace);
    }

    float floatValue() {
        return utils::numFromString<float>(m_sString).unwrapOr(0.0f);
    }

    double doubleValue() {
        return utils::numFromString<double>(m_sString).unwrapOr(0.0);
    }

    int intValue() {
        return utils::numFromString<int>(m_sString).unwrapOr(0);
    }

    unsigned int uintValue() {
        return utils::numFromString<unsigned int>(m_sString).unwrapOr(0);
    }
};

GLOBED_DLL bool CCString_initHook(CCString* self, const char* format, va_list args) {
    // check if we can cheat, %i is a very common case and we can do it faster
    std::string_view fmtstr{format};
    if (fmtstr == "%i" || fmtstr == "%d") {
        va_list tmplist;
        va_copy(tmplist, args);
        int value = va_arg(tmplist, int);
        va_end(tmplist);

        fmt::format_int f{value};
        self->m_sString = gd::string(f.data(), f.size());
        return true;
    }

    // Note: this size excludes the null terminator
    int size = std::vsnprintf(nullptr, 0, format, args);

#ifndef GEODE_IS_ANDROID
    self->m_sString = gd::string(size, '\0');
#else
    self->m_sString = gd::string(std::string(size, '\0'));
#endif

    std::vsnprintf(self->m_sString.data(), self->m_sString.size() + 1, format, args);

    return true;
}

GLOBED_DLL CCString* CCString_createHook(const char* format, ...) {
    auto ret = new CCString();
    ret->autorelease();

    va_list args;
    va_start(args, format);
    CCString_initHook(ret, format, args);
    va_end(args);

    return ret;
}

}

$execute {
    void* initAddr = nullptr;
    void* createAddr = nullptr;

#ifdef GEODE_IS_WINDOWS
    initAddr = reinterpret_cast<void*>(GetProcAddress(
        GetModuleHandleW(L"libcocos2d.dll"),
        "?initWithFormatAndValist@CCString@cocos2d@@AEAA_NPEBDPEAD@Z")
    );
#elif defined(GEODE_IS_ANDROID)
    void* handle = dlopen("libcocos2dcpp.so", RTLD_LAZY | RTLD_NOLOAD);
    initAddr = dlsym(handle, "_ZN7cocos2d8CCString23initWithFormatAndValistEPKcSt9__va_list");
#elif defined(GEODE_IS_MACOS)
    // static_assert(GEODE_COMP_GD_VERSION == 22081, "Update function");
    // // createWithFormat
    // createAddr = (void*)(geode::base::get() +
    //     GEODE_ARM_MAC(0x6b2048)
    //     GEODE_INTEL_MAC(0x7ab580)
    // );

    // tulip does not support var arg functions and macos has the init inlined :)
    return;
#else
    static_assert(GEODE_COMP_GD_VERSION == 22081, "Update function");
    // initWithFormatAndValist ios
    initAddr = (void*)(geode::base::get() + 0x268bbc);
#endif

    if (initAddr) {
        auto hook = Mod::get()->hook(
            initAddr,
            &globed::CCString_initHook,
            "cocos2d::CCString::initWithFormatAndValist"
        ).unwrap();
        hook->setPriority(Priority::Replace);
    } else {
        log::error("Failed to find CCString function to hook");
    }
}
