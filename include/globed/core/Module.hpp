#pragma once

#include <Geode/Result.hpp>
#include <Geode/utils/terminate.hpp>

namespace globed {

class Core;
struct GlobedGJBGL;

enum class AutoEnableMode {
    /// The module will never be automatically enabled. You must call `enable()` manually.
    Never,
    /// The module will be enabled when the game loads, and never disabled or re-enabled.
    Launch,
    /// The module will be enabled when the user connects to a server, and will be disabled when the user disconnects.
    /// This is the default mode.
    Server,

    Default = Server
};

class Module {
public:
    inline Module() : m_core(nullptr), m_autoEnableMode(AutoEnableMode::Default), m_enabled(false) {
        std::fill(std::begin(_reserved), std::end(_reserved), 0);
    }

    inline virtual ~Module() {}

    /// This should return the human readable name of the module.
    virtual std::string_view name() const = 0;
    /// This should return the unique identifier of the module, in format `developer.name`, just like Geode mod IDs.
    virtual std::string_view id() const = 0;
    /// This returns the name of the module's author(s).
    virtual std::string_view author() const = 0;
    /// This may return a short description of the module if overriden.
    virtual std::string_view description() const {
        return "";
    }

    /// Sets the auto-enable mode of the module. See the `AutoEnableMode` enum for mode details.
    /// Inline function; can be called without linking.
    inline void setAutoEnableMode(AutoEnableMode mode) {
        m_autoEnableMode = mode;
    }

    /// Adds a hook to the module. The hook will be enabled/disabled together with the module.
    /// The module will be responsible for managing this hook, do not use it manually.
    /// Inline function; can be called without linking.
    inline void claimHook(geode::Hook* hook) {
        this->assertNotAdded(hook);
        hook->setAutoEnable(false);
        m_hooks.push_back(hook);
    }

    /// Adds a patch to the module. The patch will be enabled/disabled together with the module.
    /// The module will be responsible for managing this patch, do not use it manually.
    /// Inline function; can be called without linking.
    inline void claimPatch(geode::Patch* patch) {
        this->assertNotAdded(patch);
        patch->setAutoEnable(false);
        m_patches.push_back(patch);
    }

    /// Enables the module. If the module is already enabled, returns `Ok()`.
    /// If the module's initialization function fails, returns an error and the module will not be enabled.
    /// Terminates the game if the module is not registered with the core. Only call after calling `Core::addModule()`.
    /// This function can only be called if linking to Globed.
    geode::Result<> enable();

    /// Disables the module. If the module is already disabled, returns `Ok()`.
    /// If the module's deinitialization function fails, returns an error.
    /// Terminates the game if the module is not registered with the core. Only call after calling `Core::addModule()`.
    /// This function can only be called if linking to Globed.
    geode::Result<> disable();

protected:
    /// Called when the module is enabled. Override this to perform any initialization logic.
    virtual geode::Result<> onEnabled() {
        return geode::Ok();
    }

    /// Called when the module is disabled. Override this to perform any cleanup logic.
    virtual geode::Result<> onDisabled() {
        return geode::Ok();
    }

    /// Called when the user joins a level while connected to a server.
    virtual void onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) {}
    /// Called when the user leaves a level or gets disconnected from a server.
    /// Only called if `onJoinLevel` was called before.
    virtual void onLeaveLevel(GlobedGJBGL* gjbgl, bool editor) {}

private:
    friend class Core;
    friend class CoreImpl;

    Core* m_core;
    AutoEnableMode m_autoEnableMode;
    bool m_enabled;
    std::vector<geode::Hook*> m_hooks;
    std::vector<geode::Patch*> m_patches;

    uint8_t _reserved[59];

    void assertCore() const;
    geode::Result<> enableHooks();
    geode::Result<> disableHooks();

    void assertNotAdded(geode::Hook* hook) const {
        if (std::find(m_hooks.begin(), m_hooks.end(), hook) != m_hooks.end()) {
            geode::utils::terminate(
                fmt::format("Globed module {} ({}) by {} tried to claim a hook that was already claimed ({})",
                            this->name(), this->id(), this->author(), hook->getDisplayName()),
                geode::Mod::get()
            );
        }
    }

    void assertNotAdded(geode::Patch* patch) const {
        if (std::find(m_patches.begin(), m_patches.end(), patch) != m_patches.end()) {
            geode::utils::terminate(
                fmt::format("Globed module {} ({}) by {} tried to claim a patch that was already claimed ({:X})",
                            this->name(), this->id(), this->author(), patch->getAddress()),
                geode::Mod::get()
            );
        }
    }
};

// static_assert(sizeof(Module) == 80);

}