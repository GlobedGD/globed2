#include "singleton.hpp"

#include <stdexcept>

#ifdef GLOBED_ENABLE_STACKTRACE
# include <cpptrace/cpptrace.hpp>
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

#ifdef GLOBED_ENABLE_STACKTRACE
        log::warn("\n{}", cpptrace::generate_trace().to_string(true));
#endif
        throw singleton_use_after_dtor(name);
    }

    void scheduleUpdateFor(cocos2d::CCObject* obj) {
        CCScheduler::get()->scheduleUpdateForTarget(obj, 0, false);
    }
}
