#pragma once

#include <globed/util/singleton.hpp>

namespace globed {

class GLOBED_DLL EmoteManager : public SingletonBase<EmoteManager> {
public:
    void registerEmote(uint32_t id, std::string name);
    void registerSfx(uint32_t id, const std::filesystem::path& name);

    cocos2d::CCSprite* createEmote(uint32_t id);
    std::unordered_map<uint32_t, std::string>& getEmotes();
    std::vector<uint32_t>& getSortedEmoteIds();

    FMOD::Channel* playEmoteSfx(uint32_t id);

protected:
    std::unordered_map<uint32_t, std::string> m_emoteNames;
    std::unordered_map<uint32_t, std::string> m_sfxPaths;
    std::vector<uint32_t> m_sortedEmoteIds;

    friend class SingletonBase;
    EmoteManager() {}
    ~EmoteManager() {}
};

}