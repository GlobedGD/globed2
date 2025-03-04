#include "singleton.hpp"

#include <stdexcept>

#ifdef GLOBED_DEBUG
# include <cpptrace/cpptrace.hpp>
#endif

using namespace geode::prelude;

class singleton_use_after_dtor : public std::runtime_error {
public:
    singleton_use_after_dtor() : std::runtime_error("attempting to use a singleton after static destruction") {}
};

namespace globed {
    void destructedSingleton() {
#ifdef GLOBED_DEBUG
        log::warn("Singleton used after static destruction!");
        log::warn("\n{}", cpptrace::generate_trace().to_string(true));
#endif
        throw singleton_use_after_dtor();
    }
}
