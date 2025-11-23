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

#ifdef GLOBED_DEBUG
$on_mod(Loaded) {
    log::info("===========================================");
    log::info("Globed loaded in debug mode");
    log::info("Globed commit: {}, Geode commit: {}", globed::constant<"globed-commit">(), globed::constant<"geode-commit">());
    log::info("Build date: {} (imprecise)", globed::constant<"build-time">());
    log::info("Build environment: {}", globed::constant<"build-env">());
    log::info("Build options: {}", globed::constant<"build-opts">());
    log::info("===========================================");
}
#endif