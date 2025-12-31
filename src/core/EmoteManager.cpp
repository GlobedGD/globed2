#include <globed/core/EmoteManager.hpp>
#include <globed/audio/AudioManager.hpp>
#include <asp/time.hpp>
#include <asp/format.hpp>
#include <asp/fs.hpp>

using namespace geode::prelude;
using namespace asp::time;

namespace globed {

static bool isValid(CCSprite* spr) {
    return spr && !spr->getUserObject("geode.texture-loader/fallback");
}

void EmoteManager::registerEmote(uint32_t id, std::string name) {
    m_emoteNames[id] = std::move(name);

    auto pos = std::lower_bound(
        m_sortedEmoteIds.begin(),
        m_sortedEmoteIds.end(),
        id
    );
    m_sortedEmoteIds.insert(pos, id);
}

void EmoteManager::registerSfx(uint32_t id, const std::filesystem::path& name) {
    m_sfxPaths[id] = utils::string::pathToString(name);
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

FMOD::Channel* EmoteManager::playEmoteSfx(uint32_t id) {
    if (!globed::setting<bool>("core.player.quick-chat-sfx")) return nullptr;

    auto it = m_sfxPaths.find(id);
    if (it == m_sfxPaths.end()) {
        return nullptr;
    }

    auto& am = AudioManager::get();
    auto res = am.createSound(it->second);
    if (!res) {
        log::warn("Failed to create emote sfx {}: {}", id, res.unwrapErr());
        return nullptr;
    }

    auto res2 = am.playSound(*res);
    if (!res2) {
        log::warn("Failed to play emote sfx {}: {}", id, res2.unwrapErr());
        return nullptr;
    }

    return *res2;
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

    char buf[256];

    for (auto& [start, count] : ranges) {
        for (auto i = start; i < start + count; i++) {
            auto name = asp::local_format(buf, "emote_{}.png"_spr, i);

            if (auto frame = sfc->spriteFrameByName(name.data())) {
                if (frame->getTag() != 105871529) { // textureldr magic number
                    em.registerEmote(i, std::string(name));
                }
            }
        }
    }

    // register sfx
    if (auto res = asp::fs::iterdir(Mod::get()->getResourcesDir())) {
        for (auto& entry : *res) {
            auto name = utils::string::pathToString(entry.path().filename());

            if (!name.starts_with("emote_sfx_") || !name.ends_with(".ogg")) {
                continue;
            }

            // extract the number
            size_t offset = sizeof("emote_sfx_") - 1;
            size_t len = name.length() - offset - (sizeof(".ogg") - 1);
            auto numStr = std::string_view{name}.substr(offset, len);
            auto num = utils::numFromString<uint32_t>(numStr).unwrapOr((uint32_t)-1);

            if (num == (uint32_t)-1) continue;

            em.registerSfx(num, entry.path());
        }
    }

    log::debug("Added {} emotes in {}", em.getEmotes().size(), now.elapsed().toString());
}

}
