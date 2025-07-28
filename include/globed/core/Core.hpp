#pragma once

#include <globed/util/singleton.hpp>
#include "Module.hpp"
#include <memory>

namespace globed {

class CoreImpl;

class Core : public SingletonBase<Core> {
public:
    /// Installs a module into the core. This does not immediately enable the module,
    /// that will only happen at the end of game loading (unless the module is explicitly disabled, then it won't happen at all).
    /// Returns an error if the module is already registered, or an internal failure occurs.
    template <typename T> requires (std::is_base_of_v<Module, std::decay_t<T>>)
    geode::Result<std::shared_ptr<Module>> addModule(T&& mod) {
        auto ptr = std::static_pointer_cast<Module>(std::make_shared<std::decay_t<T>>(std::forward<T>(mod)));
        GEODE_UNWRAP(this->addModule(ptr));
        return geode::Ok(ptr);
    }

private:
    friend class SingletonBase;
    friend class Module;
    friend class CoreImpl;

    std::unique_ptr<CoreImpl> m_impl;

    Core();
    ~Core();

    geode::Result<> addModule(std::shared_ptr<Module> mod);
};

}
