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
    log::debug("Disabling Server and Level modules");

    this->disableIf([](const Module& mod) {
        return mod.m_autoEnableMode == AutoEnableMode::Server || mod.m_autoEnableMode == AutoEnableMode::Level;
    });
}

void CoreImpl::onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) {
    log::debug("Enabling Level modules");
    this->enableIf([](const Module& mod) {
        return mod.m_autoEnableMode == AutoEnableMode::Level;
    });

    this->forEachEnabled([&](Module& mod) {
        mod.onJoinLevel(gjbgl, level, editor);
    });
}

void CoreImpl::onJoinLevelPostInit(GlobedGJBGL* gjbgl) {
    this->forEachEnabled([&](Module& mod) {
        mod.onJoinLevelPostInit(gjbgl);
    });
}

void CoreImpl::onLeaveLevel(GlobedGJBGL* gjbgl, bool editor) {
    log::debug("Disabling Level modules");
    this->disableIf([](const Module& mod) {
        return mod.m_autoEnableMode == AutoEnableMode::Level;
    });

    this->forEachEnabled([&](Module& mod) {
        mod.onLeaveLevel(gjbgl, editor);
    });
}

void CoreImpl::onPlayerJoin(GlobedGJBGL* gjbgl, int accountId) {
    this->forEachEnabled([&](Module& mod) {
        mod.onPlayerJoin(gjbgl, accountId);
    });
}

void CoreImpl::onPlayerLeave(GlobedGJBGL* gjbgl, int accountId) {
    this->forEachEnabled([&](Module& mod) {
        mod.onPlayerLeave(gjbgl, accountId);
    });
}

void CoreImpl::onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) {
    this->forEachEnabled([&](Module& mod) {
        mod.onPlayerDeath(gjbgl, player, death);
    });
}

}