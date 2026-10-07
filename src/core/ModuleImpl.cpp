#include "ModuleImpl.hpp"

using namespace geode::prelude;

namespace globed {

Result<ModuleImpl> ModuleImpl::create(ModuleVTable vtable) {
    auto metadata = vtable.getMetadata();

    if (metadata.id.empty()) {
        return Err("Module has malformed metadata (no ID)");
    }

    return Ok(ModuleImpl {
        std::move(vtable)
    });
}

std::string_view ModuleImpl::id() const {
    return m_id;
}

}
