#pragma once

#include <Geode/Result.hpp>

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
    Module();
    virtual ~Module();

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
    void setAutoEnableMode(AutoEnableMode mode);

    /// Adds a hook to the module. The hook will be enabled/disabled together with the module.
    /// The module will be responsible for managing this hook, do not use it manually.
    void claimHook(geode::Hook* hook);

    /// Adds a patch to the module. The patch will be enabled/disabled together with the module.
    /// The module will be responsible for managing this patch, do not use it manually.
    void claimPatch(geode::Patch* patch);

    /// Enables the module. If the module is already enabled, returns `Ok()`.
    /// If the module's initialization function fails, returns an error and the module will not be enabled.
    /// Terminates the game if the module is not registered with the core. Only call after calling `Core::addModule()`.
    geode::Result<> enable();

    /// Disables the module. If the module is already disabled, returns `Ok()`.
    /// If the module's deinitialization function fails, returns an error.
    /// Terminates the game if the module is not registered with the core. Only call after calling `Core::addModule()`.
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
    void assertNotAdded(geode::Hook*) const;
    void assertNotAdded(geode::Patch*) const;
    geode::Result<> enableHooks();
    geode::Result<> disableHooks();
};

// static_assert(sizeof(Module) == 80);

}