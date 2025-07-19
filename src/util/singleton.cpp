#include <stdexcept>
#include <string_view>
#include <fmt/format.h>
#include <Geode/loader/Log.hpp>

using namespace geode::prelude;

class singleton_use_after_dtor : public std::runtime_error {
public:
    singleton_use_after_dtor(std::string_view name)
    : std::runtime_error(fmt::format("attempting to use a singleton after static destruction: {}", name)) {}
};

namespace globed {
    void destructedSingleton(std::string_view name) {
        log::warn("Singleton {} used after static destruction!", name);

        throw singleton_use_after_dtor(name);
    }
}
