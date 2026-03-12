#include <globed/core/Core.hpp>
#include "CoreImpl.hpp"

#ifdef QUNET_TLS_SUPPORT
# include <xtls/Backend.hpp>
#endif

using namespace geode::prelude;

namespace globed {

Core::Core() : m_impl(std::make_unique<CoreImpl>()) {}

Core::~Core() {}

Result<> Core::addModule(std::shared_ptr<Module> mod) {
    mod->m_core = this;
    return m_impl->addModule(std::move(mod));
}

}

$on_mod(Loaded) {
#ifdef GLOBED_DEBUG
    log::info("===========================================");
    log::info("Globed loaded in debug mode");
    log::info("Globed commit: {}, Geode commit: {}", globed::constant<"globed-commit">(), globed::constant<"geode-commit">());
    log::info("Build date: {} (imprecise)", globed::constant<"build-time">());
    log::info("Build environment: {}", globed::constant<"build-env">());
    log::info("Build options: {}", globed::constant<"build-opts">());
#ifdef QUNET_TLS_SUPPORT
    log::info("TLS backend: {}", xtls::Backend::get().description());
#endif
    log::info("===========================================");
#else
    // be more brief, don't spam the log
    log::info("Globed loaded, commit: {}, Geode: {}", globed::constant<"globed-commit">(), globed::constant<"geode-commit">());
#endif
}