#pragma once

#include <core/game/Interpolator.hpp>
#include <globed/core/Core.hpp>
#include <globed/core/Module.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <memory>

#include <std23/function_ref.h>

namespace globed {

class RemotePlayer;

class CoreImpl {
public:
    CoreImpl();
    ~CoreImpl();

    static CoreImpl &get()
    {
        return *Core::get().m_impl;
    }

    geode::Result<> addModule(std::shared_ptr<Module> mod);
    Module *findModule(std::string_view id);

    // Call at the end of the loading phase, enables all modules that have auto enable set to Launch
    void onLaunch();

    // Call when connected to a server, enables all modules that have auto enable set to Server
    void onServerConnected();

    // Call when disconnected from a server, disables all modules that have auto enable set to Server
    void onServerDisconnected();

    // Call when joining a level while connected to a server, enables all modules that have auto enable set to Level
    // Also calls `onJoinLevel` on each enabled module
    void onJoinLevel(GlobedGJBGL *gjbgl, GJGameLevel *level, bool editor);
    void onJoinLevelPostInit(GlobedGJBGL *gjbgl);

    // Call when leaving a level, disables all modules that have auto enable set to Level
    void onLeaveLevel(GlobedGJBGL *gjbgl, bool editor);

    // Call when another player joins the level, calls `onPlayerJoin` on each enabled module
    void onPlayerJoin(GlobedGJBGL *gjbgl, int accountId);
    // Call when another player leaves the level, calls `onPlayerLeave` on each enabled module
    void onPlayerLeave(GlobedGJBGL *gjbgl, int accountId);
    // Call when another player dies, calls `onPlayerDeath` on each enabled module
    void onPlayerDeath(GlobedGJBGL *gjbgl, RemotePlayer *player, const PlayerDeath &death);
    void onUserlistSetup(cocos2d::CCNode *container, int accountId, bool myself, UserListPopup *popup);

    bool shouldSpeedUpNewBest(GlobedGJBGL *gjbgl);

private:
    std::vector<std::shared_ptr<Module>> m_modules;
    std::optional<MessageListenerImpl<msg::CentralLoginOkMessage> *> m_loginOkListener;

    void enableIf(std23::function_ref<bool(Module &)> &&func);
    void disableIf(std23::function_ref<bool(Module &)> &&func);
    void forEachEnabled(std23::function_ref<void(Module &)> &&func);
};

} // namespace globed