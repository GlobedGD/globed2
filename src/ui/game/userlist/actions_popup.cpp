#include "actions_popup.hpp"

#include <audio/voice_playback_manager.hpp>
#include <hooks/play_layer.hpp>
#include <managers/profile_cache.hpp>
#include <managers/block_list.hpp>
#include <managers/settings.hpp>
#include <util/ui.hpp>

using namespace geode::prelude;

bool GlobedUserActionsPopup::setup(int accountId) {
    this->accountId = accountId;

    auto& pcm = ProfileCacheManager::get();
    auto data = pcm.getData(accountId);

    std::string name = "Player";
    if (data.has_value()) {
        name = data->name;
    }

    this->setTitle(name);

    this->remakeButtons();

    return true;
}

void GlobedUserActionsPopup::remakeButtons() {
    if (buttonLayout) {
        buttonLayout->removeFromParent();
    }

    auto rlayout = util::ui::getPopupLayout(m_size);

    Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f))
        .pos(rlayout.center - CCPoint{0.f, 10.f})
        .parent(m_mainLayer)
        .store(buttonLayout);

    auto pl = static_cast<GlobedPlayLayer*>(PlayLayer::get());

    bool isUnblocked = pl->shouldLetMessageThrough(accountId);

    // mute button
    CCMenuItemSpriteExtra* muteButton;
    Build<CCSprite>::createSpriteName(isUnblocked ? "icon-mute.png"_spr : "icon-unmute.png"_spr)
        .intoMenuItem([this, isUnblocked](auto) {
            auto& bl = BlockListManager::get();
            isUnblocked ? bl.blacklist(accountId) : bl.whitelist(accountId);
            // mute them immediately
            auto& settings = GlobedSettings::get();
            auto& vpm = VoicePlaybackManager::get();

            if (isUnblocked) {
                vpm.setVolume(accountId, 0.f);
            } else {
                vpm.setVolume(accountId, settings.communication.voiceVolume);
            }

            // delay by 1 frame to prevent epic ccmenuitem::activate crash
            Loader::get()->queueInMainThread([this] {
                this->remakeButtons();
            });
        })
        .parent(buttonLayout)
        .store(muteButton);

    // hide button

    if (pl->m_fields->players.contains(accountId)) {
        bool isHidden = pl->m_fields->players.at(accountId)->getForciblyHidden();

        auto sprite = Build<CCSprite>::createSpriteName(isHidden ? "icon-show-player.png"_spr : "icon-hide-player.png"_spr).collect();
        util::ui::rescaleToMatch(sprite, muteButton);

        Build<CCSprite>(sprite)
            .intoMenuItem([this, isHidden, pl](auto) {
                if (!pl->m_fields->players.contains(accountId)) return;

                pl->m_fields->players.at(accountId)->setForciblyHidden(!isHidden);
                auto& bl = BlockListManager::get();
                bl.setHidden(accountId, !isHidden);

                Loader::get()->queueInMainThread([this] {
                    this->remakeButtons();
                });
            })
            .parent(buttonLayout);
    }

    buttonLayout->updateLayout();
}

GlobedUserActionsPopup* GlobedUserActionsPopup::create(int accountId) {
    auto ret = new GlobedUserActionsPopup;
    if (ret->init(POPUP_WIDTH, POPUP_HEIGHT, accountId)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}