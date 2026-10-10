#pragma once
#include "API.hpp"
#include <Geode/loader/Log.hpp>
#include "../core/data/PlayerState.hpp"

namespace globed {

template <typename T>
concept ValidModuleType = requires(const T t) {
    { T::metadata } -> std::convertible_to<ModuleMetadata>;
};

template <typename T>
concept ModuleOverridesAutoEnableType = requires(const T t) {
    { T::AUTO_ENABLE } -> std::convertible_to<AutoEnableMode>;
};

template <typename Derived>
struct SoftModuleAutoInit {
    SoftModuleAutoInit() {
        Derived::_register();
    }
};


template <typename Derived, bool Leak = false>
struct GLOBED_NOVTABLE SoftModule {
    static Derived& get() {
        if constexpr (Leak) {
            static auto module = new Derived{};
            return *module;
        } else {
            static Derived module;
            return module;
        }
    }

    bool isEnabled() {
        return globed::api::module::isEnabled(_id());
    }

    /// Sets how the module is expected to be enabled/disabled.
    /// By default this is `Server`, which means the module is toggled whenever the user connects or disconnects from the Globed server.
    /// See other values of `AutoEnableMode` for more information.
    void setAutoEnableMode(AutoEnableMode mode) {
        this->runModuleOperation([mode] {
            globed::api::module::setAutoEnableMode(_id(), mode);
        });
    }

    geode::Result<> enable() {
        return globed::api::module::setEnabled(_id(), true);
    }

    geode::Result<> disable() {
        return globed::api::module::setEnabled(_id(), false);
    }

    void claimHook(geode::Hook* hook) {
        claimHooks(std::span{&hook, 1});
    }

    void claimHooks(std::vector<geode::Hook*> hooks) {
        this->runModuleOperation([hooks = std::move(hooks)] {
            globed::api::module::claimHooks(_id(), hooks);
        });
    }

    // TODO (geode v6?): use modify.getAllHooks()
    template <typename T> requires (!std::convertible_to<T&, std::span<geode::Hook* const>>)
    void claimHooks(T& modify) {
        std::vector<geode::Hook*> hooks;
        hooks.reserve(modify.m_hooks.size());
        for (auto& [k, v] : modify.m_hooks) {
            v->setAutoEnable(false);
            hooks.push_back(v.get());
        }
        return claimHooks(std::move(hooks));
    }

    void claimPatch(geode::Patch* patch) {
        return claimPatches(std::span{&patch, 1});
    }

    void claimPatches(std::vector<geode::Patch*> patches) {
        this->runModuleOperation([patches = std::move(patches)] {
            globed::api::module::claimPatches(_id(), patches);
        });
    }

    // -- Callbacks --
    // Override any of those functions to automatically register a callback that will be called by Globed.


    /// Called after the module is registered. This can be used for some late setup, because unlike the constructor,
    /// it is guaranteed that this is called after Globed has been fully loaded.
    virtual void onRegistered() {}

    /// Called when the module is being enabled. This is called right before enabling any hooks/patches of this module,
    /// and can be overriden for extra session setup. If you return an error in this method, the enablement is cancelled and an error is logged.
    ///
    /// If the default enable mode is unchanged (via `setAutoEnableMode`), modules are enabled when connected to a Globed central server,
    /// and then disabled upon full disconnect.
    virtual geode::Result<> onEnabled() { return geode::Ok(); }

    /// Called when the module is being disabled. This is called after hooks/patches of the module are already disabled,
    /// and can be overriden for extra session cleanup.
    ///
    /// If the default enable mode is unchanged (via `setAutoEnableMode`), the module will be disabled upon disconnecting from the Globed central server.
    virtual void onDisabled() {}

    /// Called when the user joins a level while connected to a server.
    virtual void onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) {}
    /// Called when the user joins a level while connected to a server, after the layer has been initialized.
    virtual void onJoinLevelPostInit(GlobedGJBGL* gjbgl) {}
    /// Called when the user leaves a level or gets disconnected from a server.
    /// Only called if `onJoinLevel` was called before.
    virtual void onLeaveLevel(GlobedGJBGL* gjbgl, bool editor) {}

    /// Called when another player joins the level. Only called if `onJoinLevel` was called before.
    virtual void onPlayerJoin(GlobedGJBGL* gjbgl, int accountId) {}
    /// Called when another player leaves the level. Only called if `onPlayerJoin` was called with this player before.
    virtual void onPlayerLeave(GlobedGJBGL* gjbgl, int accountId) {}
    /// Called when another player dies on the level.
    virtual void onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) {}
    /// Called when another player respawns on the level.
    virtual void onPlayerRespawn(GlobedGJBGL* gjbgl, RemotePlayer* player) {}

    /// Return true if respawns should be synced (e.g. if room host has Faster Reset enabled, all players are affected)
    virtual bool wantsSyncReset() { return false; }

    /// Called every frame when in a level, this is the "post update", aka it runs after GJBGL update has ran this frame.
    /// This is the recommended place for most things that are to run every frame. `onPreUpdate` can be used if a different timing is needed.
    virtual void onUpdate(GlobedGJBGL* gjbgl, float dt) {}

    /// Like `onUpdate`, but runs before original GJBGL update.
    virtual void onPreUpdate(GlobedGJBGL* gjbgl, float dt) {}

    /// Called when a new best popup is shown to the user. Return `true` to make the popup not take extra time, so everyone respawns in sync.
    virtual bool shouldSpeedUpNewBest(GlobedGJBGL* gjbgl) { return false; }
    /// Called when the local player dies. Not called for anticheat spike, but is called for fake deaths.
    virtual void onLocalPlayerDeath(GlobedGJBGL* gjbgl, bool real) {}

private:
    // automatically register when the mod is loaded
    static inline SoftModuleAutoInit<Derived> s_autoInit;
    static inline auto s_autoInitRef = &SoftModule::s_autoInit;
    friend struct SoftModuleAutoInit<Derived>;
    friend Derived;

    static inline std::vector<geode::Function<void()>> s_queuedOps;
    static inline bool s_registered = false;

    SoftModule() = default;

    static void _register();
    static void _doRegister();
    static inline const ModuleMetadata& _metadata() noexcept;
    static constexpr ModuleVTable* _vtable() noexcept;
    static std::string_view _id() noexcept {
        return _metadata().id;
    }

    template <typename F>
    static void runModuleOperation(F&& op) {
        if (s_registered) {
            op();
        } else {
            s_queuedOps.push_back(std::forward<F>(op));
        }
    }

    static void runQueuedOps() {
        for (auto& op : s_queuedOps) {
            op();
        }
        s_queuedOps.clear();
    }
};


template <typename Derived, bool Leak>
void SoftModule<Derived, Leak>::_register() {
    static_assert(ValidModuleType<Derived>, "module struct is ill-formed, see the Globed documentation");

    Derived::get(); // initialize the module

    // wait until this mod is loaded (not Globed),
    // allows us to avoid static init issues
    geode::ModStateEvent(geode::ModEventType::Loaded, geode::Mod::get())
        .listen([]() {
            globed::api::waitForGlobed([] {
                _doRegister();
            });
        })
        .leak();
}


template <typename Derived, bool Leak>
void SoftModule<Derived, Leak>::_doRegister() {
    bool result = globed::api::module::registerModule(_vtable());
    if (!result) {
        geode::log::error("Failed to register module '{}' in Globed!", _id());
    } else {
        s_registered = true;

        if constexpr (ModuleOverridesAutoEnableType<Derived>) {
            globed::api::module::setAutoEnableMode(_id(), Derived::AUTO_ENABLE);
        }

        runQueuedOps();
    }
}

template <typename Derived, bool Leak>
const ModuleMetadata& SoftModule<Derived, Leak>::_metadata() noexcept {
    return Derived::metadata;
}

template <typename Derived, bool Leak>
constexpr ModuleVTable* SoftModule<Derived, Leak>::_vtable() noexcept {
    static auto vtable = []{
        ModuleVTable vtable{};
        auto tbl = &vtable;

        GLOBED_VTABLE_INIT(tbl, getMetadata, () -> ModuleMetadata {
            return _metadata();
        });

        GLOBED_VTABLE_INIT(tbl, onRegistered, () {
            return get().onRegistered();
        });

        GLOBED_VTABLE_INIT(tbl, onEnabled, () {
            return get().onEnabled();
        });

        GLOBED_VTABLE_INIT(tbl, onDisabled, () {
            return get().onDisabled();
        });

        GLOBED_VTABLE_INIT(tbl, onJoinLevel, (GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) {
            return get().onJoinLevel(gjbgl, level, editor);
        });

        GLOBED_VTABLE_INIT(tbl, onJoinLevelPostInit, (GlobedGJBGL* gjbgl) {
            return get().onJoinLevelPostInit(gjbgl);
        });

        GLOBED_VTABLE_INIT(tbl, onLeaveLevel, (GlobedGJBGL* gjbgl, bool editor) {
            return get().onLeaveLevel(gjbgl, editor);
        });

        GLOBED_VTABLE_INIT(tbl, onPlayerJoin, (GlobedGJBGL* gjbgl, int accountId) {
            return get().onPlayerJoin(gjbgl, accountId);
        });

        GLOBED_VTABLE_INIT(tbl, onPlayerLeave, (GlobedGJBGL* gjbgl, int accountId) {
            return get().onPlayerLeave(gjbgl, accountId);
        });

        GLOBED_VTABLE_INIT(tbl, onPlayerDeath, (GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death) {
            return get().onPlayerDeath(gjbgl, player, death);
        });

        GLOBED_VTABLE_INIT(tbl, onPlayerRespawn, (GlobedGJBGL* gjbgl, RemotePlayer* player) {
            return get().onPlayerRespawn(gjbgl, player);
        });

        GLOBED_VTABLE_INIT(tbl, wantsSyncReset, () {
            return get().wantsSyncReset();
        });

        GLOBED_VTABLE_INIT(tbl, onUpdate, (GlobedGJBGL* gjbgl, float dt) {
            return get().onUpdate(gjbgl, dt);
        });

        GLOBED_VTABLE_INIT(tbl, onPreUpdate, (GlobedGJBGL* gjbgl, float dt) {
            return get().onPreUpdate(gjbgl, dt);
        });

        GLOBED_VTABLE_INIT(tbl, shouldSpeedUpNewBest, (GlobedGJBGL* gjbgl) {
            return get().shouldSpeedUpNewBest(gjbgl);
        });

        GLOBED_VTABLE_INIT(tbl, onLocalPlayerDeath, (GlobedGJBGL* gjbgl, bool real) {
            return get().onLocalPlayerDeath(gjbgl, real);
        });

        return vtable;
    }();
    return &vtable;
}

}
