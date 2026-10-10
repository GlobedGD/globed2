#include "Core.hpp"
#include <core/net/NetworkManagerImpl.hpp>

#ifdef QUNET_TLS_SUPPORT
# include <xtls/Backend.hpp>
#endif

using namespace geode::prelude;

namespace globed {

GLOBED_EXPORT_SINGLETON(Core, SingletonBase<Core>);

Core::Core() {
    NetworkManagerImpl::get().listenGlobal<msg::CentralLoginOkMessage>([this](const auto& msg) {
        this->onServerConnected();
    });
}

bool Core::addModule(std::shared_ptr<ModuleImpl> mod) {
    auto it = m_modules.find(mod->id());
    if (it != m_modules.end()) {
        return false;
    }
    log::info("Registered module {}", mod->id());
    m_modules.insert(it, {std::string{mod->id()}, std::move(mod)});
    return true;
}

ModuleImpl* Core::findModule(std::string_view id) {
    auto it = m_modules.find(id);
    if (it != m_modules.end()) {
        return it->second.get();
    }
    return nullptr;
}

void Core::forEachEnabled(geode::FunctionRef<void(ModuleImpl&)> callback) {
    for (auto& [id, mod] : m_modules) {
        if (mod->isEnabled()) {
            callback(*mod);
        }
    }
}

void Core::enableIf(geode::FunctionRef<bool(ModuleImpl&)>&& func) {
    for (auto& [_, mod] : m_modules) {
        if (func(*mod)) {
            if (auto err = mod->enable().err()) {
                log::warn("Module '{}' failed to enable: {}", mod->id(), err);
            }
        }
    }
}

void Core::disableIf(geode::FunctionRef<bool(ModuleImpl&)>&& func) {
    for (auto& [_, mod] : m_modules) {
        if (func(*mod)) {
            if (auto err = mod->disable().err()) {
                log::warn("Module '{}' failed to disable: {}", mod->id(), err);
            }
        }
    }
}

void Core::onLaunch() {
    log::debug("Enabling Launch modules");

    this->enableIf([](const auto& mod) {
        return mod.getAutoEnableMode() == AutoEnableMode::Launch;
    });

    // Disallow adding any new settings
    SettingsManager::get().freeze();
}

void Core::onServerConnected() {
    log::debug("Enabling Server modules");

    this->enableIf([](const auto& mod) {
        return mod.getAutoEnableMode() == AutoEnableMode::Server;
    });
}

void Core::onServerDisconnected() {
    log::debug("Disabling Server and Level modules");

    this->disableIf([](const auto& mod) {
        auto mode = mod.getAutoEnableMode();
        return mode == AutoEnableMode::Server || mode == AutoEnableMode::Level;
    });
}

void Core::onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) {
    log::trace("Enabling Level modules");
    this->enableIf([](const auto& mod) {
        return mod.getAutoEnableMode() == AutoEnableMode::Level;
    });

    this->forEachEnabled([&](auto& mod) {
        mod.onJoinLevel(gjbgl, level, editor);
    });
}

void Core::onLeaveLevel(GlobedGJBGL* gjbgl, bool editor) {
    log::trace("Disabling Level modules");
    this->disableIf([](const auto& mod) {
        return mod.getAutoEnableMode() == AutoEnableMode::Level;
    });

    this->forEachEnabled([&](auto& mod) {
        mod.onLeaveLevel(gjbgl, editor);
    });
}

bool Core::shouldSpeedUpNewBest(GlobedGJBGL* gjbgl) {
    bool should = false;

    this->forEachEnabled([&](auto& mod) {
        should = mod.shouldSpeedUpNewBest(gjbgl) || should;
    });

    return should;
}

bool Core::wantsSyncReset() {
    bool wants = false;

    this->forEachEnabled([&](auto& mod) {
        if (mod.getAutoEnableMode() == AutoEnableMode::Level) {
            wants = mod.wantsSyncReset() || wants;
        }
    });

    return wants;
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