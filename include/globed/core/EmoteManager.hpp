#pragma once

#include <globed/util/singleton.hpp>
#include <globed/core/game/RemotePlayer.hpp>
#include <globed/audio/sound/PlayerSound.hpp>

namespace globed {

class GLOBED_DLL EmoteManager : public SingletonBase<EmoteManager> {
public:
    void registerEmote(uint32_t id, std::string name);
    void registerSfx(uint32_t id, const std::filesystem::path& name);

    cocos2d::CCSprite* createEmote(uint32_t id);
    cocos2d::CCSprite* createFavoriteEmote(uint32_t id);

    /// Returns the favorite emote by given index (from 0 to 7). Zero if not set.
    uint32_t getFavoriteEmote(uint32_t idx);

    std::unordered_map<uint32_t, std::string>& getEmotes();
    std::vector<uint32_t>& getSortedEmoteIds();

    std::shared_ptr<PlayerSound> playEmoteSfx(uint32_t id, std::shared_ptr<RemotePlayer> player);

protected:
    std::unordered_map<uint32_t, std::string> m_emoteNames;
    std::unordered_map<uint32_t, std::string> m_sfxPaths;
    std::vector<uint32_t> m_sortedEmoteIds;

    friend class SingletonBase;
    EmoteManager() {}
    ~EmoteManager() {}
};

}