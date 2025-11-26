#pragma once
#include "Module.hpp"
#include "Core.hpp"
#include <globed/util/singleton.hpp>

namespace globed {

template <typename Derived>
struct ModuleCrtpAutoInit {
    ModuleCrtpAutoInit() {
        Derived::get();
    }
};

template <typename Derived>
class ModuleCrtpBase : public Module {
public:
    using Module::Module;

    static Derived& get() {
        if (!g_instance) {
            auto res = Core::get().addModule(Derived{});

            if (!res) {
                geode::utils::terminate(fmt::format("Failed to initialize module {}: {}", globed::getTypenameConstexpr<Derived>(), res.unwrapErr()));
            }

            auto instance = res.unwrap();

            g_instance = std::static_pointer_cast<Derived>(std::move(instance));

            if constexpr (requires { g_instance->onModuleInit(); }) {
                g_instance->onModuleInit();
            }
        }

        return *g_instance;
    }

private:
    static inline std::shared_ptr<Derived> g_instance;

    // automatically initialize the module when the program starts
    static inline ModuleCrtpAutoInit<Derived> s_autoInit;
    static inline auto s_autoInitRef = &ModuleCrtpBase::s_autoInit;

};

}