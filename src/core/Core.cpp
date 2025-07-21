#include <globed/core/Core.hpp>
#include "CoreImpl.hpp"

using namespace geode::prelude;

namespace globed {

Core::Core() : m_impl(std::make_unique<CoreImpl>()) {}

Core::~Core() {}

Result<> Core::addModule(std::shared_ptr<Module> mod) {
    mod->m_core = this;
    return m_impl->addModule(std::move(mod));
}

}
