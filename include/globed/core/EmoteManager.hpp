#pragma once

#include <globed/util/singleton.hpp>

namespace globed {

class GLOBED_DLL EmoteManager : public SingletonBase<EmoteManager> {
public:
    void registerEmote(uint32_t id, std::string&& name);

    cocos2d::CCSprite* createEmote(uint32_t id);
    std::unordered_map<uint32_t, std::string>& getEmotes();
    std::vector<uint32_t>& getSortedEmoteIds();

protected:
    std::unordered_map<uint32_t, std::string> m_emoteNames;
    std::vector<uint32_t> m_sortedEmoteIds;

    friend class SingletonBase;
    EmoteManager() {}
    ~EmoteManager() {}
};

}