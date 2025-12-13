#include <stdexcept>
#include <string_view>
#include <fmt/format.h>
#include <Geode/loader/Log.hpp>

#if defined(GLOBED_DEBUG) && defined(GEODE_IS_WINDOWS)
# include <stacktrace>
#endif

using namespace geode::prelude;

class singleton_use_after_dtor : public std::runtime_error {
public:
    singleton_use_after_dtor(std::string_view name)
    : std::runtime_error(fmt::format("attempting to use a singleton after static destruction: {}", name)) {}
};

namespace globed {
    void destructedSingleton(std::string_view name) {
        log::warn("Singleton {} used after static destruction!", name);

#if defined(GLOBED_DEBUG) && defined(GEODE_IS_WINDOWS)
        auto trace = std::stacktrace::current();
        for (const auto& frame : trace) {
            auto me = (uintptr_t) GetModuleHandleA("dankmeme.globed3.dll");
            auto hnd = (uintptr_t) frame.native_handle();
            if (hnd >= me && hnd < me + 0x10000000) {
                log::warn("- globed + {:x}", hnd - me);
            } else {
                log::warn("- {:x}", hnd);
            }
        }
#endif

        throw singleton_use_after_dtor(name);
    }

    void scheduleUpdateFor(cocos2d::CCObject* obj) {
        CCScheduler::get()->scheduleUpdateForTarget(obj, 0, false);
    }
}
