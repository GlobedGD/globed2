#include "CoreImpl.hpp"
#include <globed/core/SettingsManager.hpp>

using namespace geode::prelude;

namespace globed {

CoreImpl::CoreImpl() {}
CoreImpl::~CoreImpl() {}

Result<> CoreImpl::addModule(std::shared_ptr<Module> mod) {
    if (this->findModule(mod->id())) {
        return Err("Module with ID '{}' is already registered", mod->id());
    }

    m_modules.push_back(std::move(mod));

    return Ok();
}

Module* CoreImpl::findModule(std::string_view id) {
    for (auto& mod : m_modules) {
        if (mod->id() == id) {
            return mod.get();
        }
    }

    return nullptr;
}

void CoreImpl::onLaunch() {
    log::debug("Enabling Launch modules");

    this->enableIf([](const Module& mod) {
        return mod.m_autoEnableMode == AutoEnableMode::Launch;
    });

    // Disallow adding any new settings
    SettingsManager::get().freeze();
}

void CoreImpl::onServerConnected() {
    log::debug("Enabling Server modules");

    this->enableIf([](const Module& mod) {
        return mod.m_autoEnableMode == AutoEnableMode::Server;
    });
}

void CoreImpl::onServerDisconnected() {
    log::debug("Disabling Server modules");

    this->disableIf([](const Module& mod) {
        return mod.m_autoEnableMode == AutoEnableMode::Server;
    });
}

void CoreImpl::onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) {
    this->forEachEnabled([&](Module& mod) {
        mod.onJoinLevel(gjbgl, level, editor);
    });
}

void CoreImpl::onLeaveLevel(GlobedGJBGL* gjbgl, bool editor) {
    this->forEachEnabled([&](Module& mod) {
        mod.onLeaveLevel(gjbgl, editor);
    });
}

}