#include "ModuleImpl.hpp"

using namespace geode::prelude;

namespace globed {

Result<std::shared_ptr<ModuleImpl>> ModuleImpl::create(ModuleVTable* vtable) {
    auto metadata = vtable->getMetadata();

    if (metadata.id.empty()) {
        return Err("Module has malformed metadata (no ID)");
    }

    auto module = std::make_shared<ModuleImpl>(vtable, ctor_tag{});
    module->m_id = std::move(metadata.id);
    return Ok(module);
}

std::string_view ModuleImpl::id() const {
    return m_id;
}

AutoEnableMode ModuleImpl::getAutoEnableMode() const {
    return m_autoEnableMode;
}

void ModuleImpl::setAutoEnableMode(AutoEnableMode mode) {
    m_autoEnableMode = mode;
}

bool ModuleImpl::isEnabled() const {
    return m_enabled;
}

Result<> ModuleImpl::enable() {
    if (m_enabled) return Ok();

    GEODE_UNWRAP(m_vtable->onEnabled());

    for (auto& hook : m_hooks) {
        GEODE_UNWRAP(hook->enable());
    }

    // unlike hooks, patches return an error if enabled already
    for (auto& patch : m_patches) {
        if (!patch->isEnabled()) GEODE_UNWRAP(patch->enable());
    }

    m_enabled = true;

    return Ok();
}

Result<> ModuleImpl::disable() {
    if (!m_enabled) return Ok();

    for (auto& hook : m_hooks) {
        GEODE_UNWRAP(hook->disable());
    }

    for (auto& patch : m_patches) {
        if (patch->isEnabled()) GEODE_UNWRAP(patch->disable());
    }

    m_vtable->onDisabled();
    m_enabled = false;

    return Ok();
}

void ModuleImpl::claimHooks(std::span<geode::Hook* const> hooks) {
    for (auto hook : hooks) {
        hook->setAutoEnable(false);
        m_hooks.insert(hook);
    }
}

void ModuleImpl::claimPatches(std::span<geode::Patch* const> patches) {
    for (auto patch : patches) {
        patch->setAutoEnable(false);
        m_patches.insert(patch);
    }
}

void ModuleImpl::onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) {
    m_vtable->onJoinLevel(gjbgl, level, editor);
}

void ModuleImpl::onJoinLevelPostInit(GlobedGJBGL* gjbgl) {
    m_vtable->onJoinLevelPostInit(gjbgl);
}

void ModuleImpl::onLeaveLevel(GlobedGJBGL* gjbgl, bool editor) {
    m_vtable->onLeaveLevel(gjbgl, editor);
}

void ModuleImpl::onPlayerJoin(GlobedGJBGL* gjbgl, int accountId) {
    m_vtable->onPlayerJoin(gjbgl, accountId);
}

void ModuleImpl::onPlayerLeave(GlobedGJBGL* gjbgl, int accountId) {
    m_vtable->onPlayerLeave(gjbgl, accountId);
}

void ModuleImpl::onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) {
    m_vtable->onPlayerDeath(gjbgl, player, death);
}

void ModuleImpl::onPlayerRespawn(GlobedGJBGL* gjbgl, RemotePlayer* player) {
    m_vtable->onPlayerRespawn(gjbgl, player);
}


bool ModuleImpl::shouldSpeedUpNewBest(GlobedGJBGL* gjbgl) {
    return m_vtable->shouldSpeedUpNewBest(gjbgl);
}

void ModuleImpl::onLocalPlayerDeath(GlobedGJBGL* gjbgl, bool real) {
    m_vtable->onLocalPlayerDeath(gjbgl, real);
}

void ModuleImpl::onUpdate(GlobedGJBGL* gjbgl, float dt) {
    m_vtable->onUpdate(gjbgl, dt);
}

void ModuleImpl::onPreUpdate(GlobedGJBGL* gjbgl, float dt) {
    m_vtable->onPreUpdate(gjbgl, dt);
}

bool ModuleImpl::wantsSyncReset() {
    return m_vtable->wantsSyncReset();
}


}
