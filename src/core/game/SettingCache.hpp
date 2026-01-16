#pragma once

#include <globed/core/SettingsManager.hpp>
#include <globed/util/singleton.hpp>

/// Fast cahce for game related settings

namespace globed {

struct CachedSettings {
    bool selfName = globed::setting<bool>("core.level.self-name");
    bool selfStatusIcons = globed::setting<bool>("core.level.self-status-icons");
    bool quickChat = globed::setting<bool>("core.player.quick-chat-enabled");
    bool voiceChat = globed::setting<bool>("core.audio.voice-chat-enabled");
    bool voiceLoopback = globed::setting<bool>("core.audio.voice-loopback");
    bool friendsOnlyAudio = globed::setting<bool>("core.audio.only-friends");
    bool editorEnabled = globed::setting<bool>("core.editor.enabled");
    bool ghostFollower = globed::setting<bool>("core.dev.ghost-follower");
    bool forceVisibility = globed::setting<bool>("core.player.force-visibility");
    bool hidePracticing = globed::setting<bool>("core.player.hide-practicing");
    bool hideNearbyClassic = globed::setting<bool>("core.player.hide-nearby-classic");
    bool hideNearbyPlat = globed::setting<bool>("core.player.hide-nearby-plat");
    bool showNames = globed::setting<bool>("core.player.show-names");
    bool showStatusIcons = globed::setting<bool>("core.player.status-icons");
    bool dualName = globed::setting<bool>("core.player.dual-name");
    bool rotateNames = globed::setting<bool>("core.player.rotate-names");
    bool defaultDeathEffects = globed::setting<bool>("core.player.default-death-effects");
    float playerOpacity = globed::setting<float>("core.player.opacity");
    float nameOpacity = globed::setting<float>("core.player.name-opacity");

    void reload()
    {
        *this = CachedSettings{};
    }

    static CachedSettings &get()
    {
        static CachedSettings instance;
        return instance;
    }
};

} // namespace globed
