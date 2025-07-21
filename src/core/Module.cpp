#include <globed/core/Module.hpp>
#include <Geode/utils/terminate.hpp>

using namespace geode::prelude;

namespace globed {

Module::Module()
    : m_core(nullptr),
      m_autoEnableMode(AutoEnableMode::Default),
      m_enabled(false) {}

Module::~Module() {}

Result<> Module::enable() {
    this->assertCore();

    if (this->m_enabled) {
        return Ok();
    }

    GEODE_UNWRAP(this->onEnabled());
    GEODE_UNWRAP(this->enableHooks());

    m_enabled = true;
    return Ok();
}

Result<> Module::disable() {
    this->assertCore();

    if (!this->m_enabled) {
        return Ok();
    }

    m_enabled = false;

    GEODE_UNWRAP(this->disableHooks());
    GEODE_UNWRAP(this->onDisabled());

    return Ok();
}

Result<> Module::enableHooks() {
    for (auto& hook : m_hooks) {
        // note that we dont have to check if the hook is already enabled, geode just returns Ok, unlike patches
        GEODE_UNWRAP(hook->enable());
    }

    for (auto& patch : m_patches) {
        if (!patch->isEnabled()) GEODE_UNWRAP(patch->enable());
    }

    return Ok();
}

Result<> Module::disableHooks() {
    for (auto& hook : m_hooks) {
        GEODE_UNWRAP(hook->disable());
    }

    for (auto& patch : m_patches) {
        if (patch->isEnabled()) GEODE_UNWRAP(patch->disable());
    }

    return Ok();
}

void Module::setAutoEnableMode(AutoEnableMode mode) {
    m_autoEnableMode = mode;
}

void Module::claimHook(geode::Hook* hook) {
    this->assertNotAdded(hook);
    hook->setAutoEnable(false);
    m_hooks.push_back(hook);
}

void Module::claimPatch(geode::Patch* patch) {
    this->assertNotAdded(patch);
    patch->setAutoEnable(false);
    m_patches.push_back(patch);
}

void Module::assertCore() const {
    if (!m_core) {
        geode::utils::terminate(
            fmt::format("Globed module {} ({}) by {} called a module function before registering the mod with the core",
                        this->name(), this->id(), this->author()),
            Mod::get()
        );
    }
}

void Module::assertNotAdded(geode::Hook* hook) const {
    if (std::find(m_hooks.begin(), m_hooks.end(), hook) != m_hooks.end()) {
        geode::utils::terminate(
            fmt::format("Globed module {} ({}) by {} tried to claim a hook that was already claimed ({})",
                        this->name(), this->id(), this->author(), hook->getDisplayName()),
            Mod::get()
        );
    }
}

void Module::assertNotAdded(geode::Patch* patch) const {
    if (std::find(m_patches.begin(), m_patches.end(), patch) != m_patches.end()) {
        geode::utils::terminate(
            fmt::format("Globed module {} ({}) by {} tried to claim a patch that was already claimed ({:X})",
                        this->name(), this->id(), this->author(), patch->getAddress()),
            Mod::get()
        );
    }
}


}