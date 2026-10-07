#pragma once
#include <globed/core/Module.hpp>
#include <globed/prelude.hpp>

namespace globed {

class ModuleImpl {
public:
    static Result<std::shared_ptr<ModuleImpl>> create(ModuleVTable vtable);
    std::string_view id() const;

private:
    std::string m_id;
    ModuleVTable m_vtable;

    ModuleImpl(ModuleVTable vtable) : m_vtable(std::move(vtable)) {}
};

}
