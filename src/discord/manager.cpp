#include "manager.hpp"

#if GLOBED_HAS_DRPC

using namespace discord;

#ifdef _DEBUG
#define DISCORD_LOG_LEVEL LogLevel::Debug
#else
#define DISCORD_LOG_LEVEL LogLevel::Warn
#endif

GLOBED_SINGLETON_DEF(DiscordManager);

DiscordManager::DiscordManager() {
    auto result = Core::Create(DISCORD_APP_ID, DiscordCreateFlags_Default, &core);
    
    GLOBED_REQUIRE(core != nullptr, std::string("failed to initialize discord core: error ") + std::to_string((int)result));

    geode::log::debug("created core");

    core->SetLogHook(DISCORD_LOG_LEVEL, [](LogLevel level, const char* message) {
        switch (level) {
            case LogLevel::Debug:
                geode::log::debug("[Discord]: {}", message);
            case LogLevel::Info:
                geode::log::info("[Discord]: {}", message);
            case LogLevel::Warn:
                geode::log::warn("[Discord]: {}", message);
            case LogLevel::Error:
                geode::log::error("[Discord]: {}", message);
        }
    });
    geode::log::debug("set hook");
    // line below raises 0xc0000096 STATUS_PRIVILEGED_INSTRUCTION
    auto& um = core->UserManager();
    geode::log::debug("got user manager");
    
    um.OnCurrentUserUpdate.Connect([]() {
        geode::log::debug("getting current user");
        // User user;
        // this->core->UserManager().GetCurrentUser(&user);

        // geode::log::debug("Current user: {}", user.GetUsername());
    });
    geode::log::debug("set updater");

}

void DiscordManager::update() {
    core->RunCallbacks();
}

#endif // GLOBED_HAS_DRPC