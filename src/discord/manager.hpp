#pragma once
#include <defs.hpp>

#if GLOBED_HAS_DRPC
#include <dankmeme.discord-sdk/include/discord.h>

class DiscordManager {
public:
    static const int64_t DISCORD_APP_ID = 1171754088267005963;

    GLOBED_SINGLETON(DiscordManager);
    DiscordManager();

    void update();

private:
    discord::Core* core = nullptr;
};

#endif // GLOBED_HAS_DRPC