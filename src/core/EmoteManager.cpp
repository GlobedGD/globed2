#include <globed/core/EmoteManager.hpp>
#include <asp/time.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

static bool isValid(CCSprite* spr) {
    return spr && !spr->getUserObject("geode.texture-loader/fallback");
}

void EmoteManager::registerEmote(uint32_t id, std::string&& name) {
    m_emoteNames[id] = std::move(name);

    auto pos = std::lower_bound(
        m_sortedEmoteIds.begin(),
        m_sortedEmoteIds.end(),
        id
    );
    m_sortedEmoteIds.insert(pos, id);
}

CCSprite* EmoteManager::createEmote(uint32_t id) {
    auto it = m_emoteNames.find(id);
    if (it == m_emoteNames.end()) {
        return nullptr;
    }

    auto spr = CCSprite::createWithSpriteFrameName(it->second.c_str());
    if (!isValid(spr)) {
        spr = CCSprite::create(it->second.c_str());
    }

    return isValid(spr) ? spr : nullptr;
}

std::unordered_map<uint32_t, std::string>& EmoteManager::getEmotes() {
    return m_emoteNames;
}

std::vector<uint32_t>& EmoteManager::getSortedEmoteIds() {
    return m_sortedEmoteIds;
}

$on_game(Loaded) {
    auto now = Instant::now();
    auto& em = EmoteManager::get();

    std::initializer_list<std::pair<uint32_t, uint32_t>> ranges = {
        {0, 100},
        {200, 100},
        {300, 100},
        {400, 100},
        {500, 100},
        {600, 100},
        {700, 100},
    };

    auto sfc = CCSpriteFrameCache::get();

    for (auto& [start, count] : ranges) {
        for (auto i = start; i < start + count; i++) {
            auto name = fmt::format("emote_{}.png"_spr, i);
            if (auto frame = sfc->spriteFrameByName(name.c_str())) {
                if (frame->getTag() != 105871529) {
                    em.registerEmote(i, std::move(name));
                }
            }
            // bool wasAdded = CCTextureCache::get()->addImage(name.c_str(), false);

            // if (wasAdded) {
            //     em.registerEmote(i, std::move(name));
            // }
        }
    }

    log::debug("Added {} emotes in {}", em.getEmotes().size(), now.elapsed().toString());
}

}
