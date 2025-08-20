#pragma once

#include <globed/core/Module.hpp>
#include <globed/core/Core.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <core/game/Interpolator.hpp>
#include <memory>

namespace globed {

class RemotePlayer;

class CoreImpl {
public:
    CoreImpl();
    ~CoreImpl();

    static CoreImpl& get() {
        return *Core::get().m_impl;
    }

    geode::Result<> addModule(std::shared_ptr<Module> mod);
    Module* findModule(std::string_view id);

    // Call at the end of the loading phase, enables all modules that have auto enable set to Launch
    void onLaunch();

    // Call when connected to a server, enables all modules that have auto enable set to Server
    void onServerConnected();

    // Call when disconnected from a server, disables all modules that have auto enable set to Server
    void onServerDisconnected();

    // Call when joining a level while connected to a server, enables all modules that have auto enable set to Level
    // Also calls `onJoinLevel` on each enabled module
    void onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor);
    void onJoinLevelPostInit(GlobedGJBGL* gjbgl);

    // Call when leaving a level, disables all modules that have auto enable set to Level
    void onLeaveLevel(GlobedGJBGL* gjbgl, bool editor);

    // Call when another player joins the level, calls `onPlayerJoin` on each enabled module
    void onPlayerJoin(GlobedGJBGL* gjbgl, int accountId);
    // Call when another player leaves the level, calls `onPlayerLeave` on each enabled module
    void onPlayerLeave(GlobedGJBGL* gjbgl, int accountId);
    // Call when another player dies, calls `onPlayerDeath` on each enabled module
    void onPlayerDeath(GlobedGJBGL* gjbgl, RemotePlayer* player, const PlayerDeath& death);
    void onUserlistSetup(cocos2d::CCNode* container, int accountId, bool myself, UserListPopup* popup);

    bool shouldSpeedUpNewBest(GlobedGJBGL* gjbgl);

private:
    std::vector<std::shared_ptr<Module>> m_modules;
    std::optional<MessageListenerImpl<msg::CentralLoginOkMessage>*> m_loginOkListener;

    template <typename F>
    void enableIf(F&& func) {
        for (auto& mod : m_modules) {
            if (func(*mod)) {
                if (auto err = mod->enable().err()) {
                    geode::log::warn("Module '{}' failed to enable: {}", mod->id(), err);
                }
            }
        }
    }

    template <typename F>
    void disableIf(F&& func) {
        for (auto& mod : m_modules) {
            if (func(*mod)) {
                if (auto err = mod->disable().err()) {
                    geode::log::warn("Module '{}' failed to disable: {}", mod->id(), err);
                }
            }
        }
    }

    template <typename F>
    void forEachEnabled(F&& func) {
        for (auto& mod : m_modules) {
            if (mod->m_enabled) {
                func(*mod);
            }
        }
    }
};

}