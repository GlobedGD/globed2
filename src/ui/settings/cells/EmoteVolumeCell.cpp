#include "EmoteVolumeCell.hpp"
#include <UIBuilder.hpp>
#include <globed/audio/AudioManager.hpp>
#include <globed/core/EmoteManager.hpp>

#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

void EmoteVolumeCell::setup()
{
    FloatSettingCell::setup();

    Build<CCSprite>::createSpriteName("GJ_playBtn2_001.png")
        .with([&](auto spr) { cue::rescaleToMatch(spr, m_size.height * 0.75f); })
        .intoMenuItem([this] { this->playEmote(); })
        .scaleMult(1.15f)
        .parent(m_rightMenu);

    m_rightMenu->updateLayout();
}

void EmoteVolumeCell::playEmote()
{
    if (!globed::setting<bool>("core.player.quick-chat-sfx")) {
        globed::alert("Note", "You need to enable <cy>Emote Sound Effects</c> to hear emote sound effects.");
        return;
    }

    if (globed::setting<float>("core.player.quick-chat-sfx-volume") <= 0.f) {
        globed::alert("Note", "You need to set <cy>Emote SFX Volume</c> above 0 to hear emote sound effects.");
        return;
    }

    auto sound = EmoteManager::get().playEmoteSfx(3, nullptr);

    auto &am = AudioManager::get();
    am.setDeafen(false);
    am.clearFocusedPlayer();
    am.updatePlayback({}, false);
}

EmoteVolumeCell *EmoteVolumeCell::create(CStr key, CStr name, CStr desc, CCSize cellSize)
{
    auto ret = new EmoteVolumeCell;
    if (ret->init(key, name, desc, cellSize)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

} // namespace globed