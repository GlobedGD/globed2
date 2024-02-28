#include "user_cell.hpp"

#include "userlist.hpp"
#include <audio/voice_playback_manager.hpp>
#include <data/packets/client/admin.hpp>
#include <data/packets/server/admin.hpp>
#include <managers/block_list.hpp>
#include <managers/settings.hpp>
#include <managers/profile_cache.hpp>
#include <hooks/play_layer.hpp>
#include <hooks/gjgamelevel.hpp>
#include <ui/menu/admin/user_popup.hpp>
#include <ui/general/ask_input_popup.hpp>
#include <ui/general/intermediary_loading_popup.hpp>
#include <util/format.hpp>

using namespace geode::prelude;

bool GlobedUserCell::init(const PlayerStore::Entry& entry, const PlayerAccountData& data) {
    if (!CCLayer::init()) return false;

    accountData = data;

    auto winSize = CCDirector::get()->getWinSize();

    auto gm = GameManager::get();

    auto sp = Build<SimplePlayer>::create(data.icons.cube)
        .scale(0.6f)
        .playerFrame(data.icons.cube, IconType::Cube)
        .color(gm->colorForIdx(data.icons.color1))
        .secondColor(gm->colorForIdx(data.icons.color2))
        .parent(this)
        .pos(25.f, CELL_HEIGHT / 2.f)
        .id("player-icon"_spr)
        .collect();

    if (data.icons.glowColor != -1) {
        sp->setGlowOutline(gm->colorForIdx(data.icons.glowColor));
    } else {
        sp->disableGlowOutline();
    }

    Build<CCMenu>::create()
        .pos(0.f, 0.f)
        .parent(this)
        .store(menu);

    auto& pcm = ProfileCacheManager::get();
    ccColor3B nameColor = ccc3(255, 255, 255);
    if (data.specialUserData.has_value()) {
        nameColor = data.specialUserData->nameColor;
    }

    auto* nameLabel = Build<CCLabelBMFont>::create(data.name.data(), "bigFont.fnt")
        .color(nameColor)
        .limitLabelWidth(140.f, 0.5f, 0.1f)
        .collect();

    auto* nameButton = Build<CCMenuItemSpriteExtra>::create(nameLabel, this, menu_selector(GlobedUserCell::onOpenProfile))
        // goodness
        .pos(sp->getPositionX() + nameLabel->getScaledContentSize().width / 2.f + 25.f, CELL_HEIGHT / 2.f)
        .parent(menu)
        .collect();

    // percentage label
    Build<CCLabelBMFont>::create("", "goldFont.fnt")
        .pos(nameButton->getPosition() + nameButton->getScaledContentSize() / 2.f + CCPoint{3.f, -3.f})
        .anchorPoint({0.f, 0.5f})
        .scale(0.4f)
        .parent(this)
        .id("percentage-label"_spr)
        .store(percentageLabel);

    this->makeButtons();

    this->refreshData(entry);
    this->schedule(schedule_selector(GlobedUserCell::updateVisualizer), 1.f / 60.f);

    return true;
}

void GlobedUserCell::refreshData(const PlayerStore::Entry& entry) {
    if (_data != entry) {
        _data = entry;

        bool platformer = PlayLayer::get()->m_level->isPlatformer();
        if (platformer && _data.localBest != 0) {
            percentageLabel->setString(util::format::formatPlatformerTime(_data.localBest).c_str());
        } else if (!platformer) {
            percentageLabel->setString(fmt::format("{}%", _data.localBest).c_str());
        }
    }
}

void GlobedUserCell::updateVisualizer(float dt) {
#if GLOBED_VOICE_SUPPORT
    if (audioVisualizer) {
        auto& vpm = VoicePlaybackManager::get();

        float loudness = vpm.getLoudness(accountData.accountId);
        audioVisualizer->setVolume(loudness);
    }
#endif // GLOBED_VOICE_SUPPORT
}

void GlobedUserCell::makeButtons() {
    if (buttonsWrapper) buttonsWrapper->removeFromParent();
    Build<CCMenu>::create()
        .anchorPoint(1.0f, 0.5f)
        .pos(GlobedUserListPopup::LIST_WIDTH - 7.f, CELL_HEIGHT / 2.f)
        .layout(RowLayout::create()
                    ->setGap(5.f)
                    ->setAutoScale(false)
                    ->setAxisReverse(true))
        .parent(menu)
        .store(buttonsWrapper);

    auto maxWidth = 0.f;

    if (accountData.accountId != GJAccountManager::get()->m_accountID) {
        // mute/unmute button
        auto pl = static_cast<GlobedPlayLayer*>(PlayLayer::get());
        bool isUnblocked = pl->shouldLetMessageThrough(accountData.accountId);

        Build<CCSprite>::createSpriteName(isUnblocked ? "icon-mute.png"_spr : "icon-unmute.png"_spr)
            .scale(0.475f)
            .intoMenuItem([isUnblocked, this](auto) {
                auto& bl = BlockListManager::get();
                isUnblocked ? bl.blacklist(this->accountData.accountId) : bl.whitelist(this->accountData.accountId);
                // mute them immediately
                auto& settings = GlobedSettings::get();
                auto& vpm = VoicePlaybackManager::get();

                if (isUnblocked) {
                    vpm.setVolume(this->accountData.accountId, 0.f);
                } else {
                    vpm.setVolume(this->accountData.accountId, settings.communication.voiceVolume);
                }

                this->makeButtons();
            })
            .parent(buttonsWrapper)
            .id("block-button"_spr)
            .store(blockButton);

        maxWidth += blockButton->getScaledContentSize().width + 5.f;
    }

    // admin menu button
    if (NetworkManager::get().isAuthorizedAdmin()) {
        Build<CCSprite>::createSpriteName("GJ_reportBtn_001.png")
            .scale(0.4f)
            .intoMenuItem([this](auto) {
                // load the data from the server
                auto& nm = NetworkManager::get();
                IntermediaryLoadingPopup::create([&nm, this](auto popup) {
                    nm.send(AdminGetUserStatePacket::create(std::to_string(accountData.accountId)));
                    nm.addListener<AdminUserDataPacket>([this, popup = popup](auto packet) {
                        popup->onClose(popup);

                        // delay the cration to avoid deadlock
                        Loader::get()->queueInMainThread([userEntry = packet->userEntry, accountData = accountData.makeRoomPreview(0)] {
                            AdminUserPopup::create(userEntry, accountData)->show();
                        });
                    });
                }, [&nm, this](auto) {
                    nm.removeListenerDelayed<AdminUserDataPacket>(util::time::seconds(3));
                })->show();
            })
            .parent(buttonsWrapper)
            .id("kick-button"_spr)
            .store(kickButton);

        maxWidth += kickButton->getScaledContentSize().width + 5.f;

        if (accountData.accountId != GJAccountManager::get()->m_accountID) {
            GlobedPlayLayer* gpl = static_cast<GlobedPlayLayer*>(PlayLayer::get()); // ggpl

            Build<CCSprite>::createSpriteName("icon-teleport.png"_spr)
                .scale(0.35f)
                .intoMenuItem([gpl, this](auto) {
                    static_cast<HookedGJGameLevel*>(gpl->m_level)->m_fields->shouldStopProgress = true; // turn on safe mode

                    PlayerObject* po1 = gpl->m_player1;
                    CCPoint position = {0,0}; // just in case the player has already left by the time we teleport
                    if (gpl->m_fields->interpolator->hasPlayer(accountData.accountId)) position = gpl->m_fields->interpolator->getPlayerState(accountData.accountId).player1.position;

                    po1->m_position = position;
                })
                .parent(buttonsWrapper)
                .id("teleport-button"_spr)
                .store(teleportButton);

            maxWidth += teleportButton->getScaledContentSize().width + 5.f;
        }
    }


    if (accountData.accountId != GJAccountManager::get()->m_accountID) {
        // audio visualizer
        Build<GlobedAudioVisualizer>::create()
#if !GLOBED_VOICE_SUPPORT
            .visible(false)
#endif // GLOBED_VOICE_SUPPORT
            .parent(buttonsWrapper)
            .id("audio-visualizer"_spr)
            .store(audioVisualizer);

        audioVisualizer->setScaleX(0.5f);

        maxWidth += audioVisualizer->getScaledContentSize().width;
    }

    buttonsWrapper->setContentSize({maxWidth, 20.f});

    buttonsWrapper->updateLayout();
}

void GlobedUserCell::onOpenProfile(CCObject*) {
    bool myself = accountData.accountId == GJAccountManager::get()->m_accountID;
    if (!myself) {
        GameLevelManager::sharedState()->storeUserName(accountData.userId, accountData.accountId, accountData.name);
    }

    ProfilePage::create(accountData.accountId, myself)->show();
}

GlobedUserCell* GlobedUserCell::create(const PlayerStore::Entry& entry, const PlayerAccountData& data) {
    auto ret = new GlobedUserCell;
    if (ret->init(entry, data)) {
        return ret;
    }

    delete ret;
    return nullptr;
}