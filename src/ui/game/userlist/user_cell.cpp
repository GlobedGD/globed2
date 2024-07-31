#include "user_cell.hpp"

#include "userlist.hpp"
#include "actions_popup.hpp"
#include <audio/voice_playback_manager.hpp>
#include <managers/admin.hpp>
#include <managers/block_list.hpp>
#include <managers/settings.hpp>
#include <managers/profile_cache.hpp>
#include <managers/friend_list.hpp>
#include <hooks/gjbasegamelayer.hpp>
#include <hooks/gjgamelevel.hpp>
#include <ui/general/ask_input_popup.hpp>
#include <util/format.hpp>
#include <util/ui.hpp>
#include <util/gd.hpp>

using namespace geode::prelude;

bool GlobedUserCell::init(const PlayerStore::Entry& entry, const PlayerAccountData& data, GlobedUserListPopup* parent) {
    if (!CCLayer::init()) return false;

    this->parent = parent;
    accountData = data;

    auto winSize = CCDirector::get()->getWinSize();

    auto gm = GameManager::get();
    auto& flm = FriendListManager::get();
    this->isFriend = flm.isFriend(data.accountId);

    Build<CCMenu>::create()
        .pos(0.f, 0.f)
        .parent(this)
        .store(menu);

    Build<CCMenu>::create()
        .layout(RowLayout::create()->setAutoScale(false)->setAxisAlignment(AxisAlignment::Start))
        .anchorPoint(0.f, 0.5f)
        .pos(5.f, CELL_HEIGHT / 2.f)
        .contentSize(GlobedUserListPopup::LIST_WIDTH, CELL_HEIGHT)
        .parent(menu)
        .id("name-layout"_spr)
        .store(usernameLayout);

    auto sp = Build<GlobedSimplePlayer>::create(data.icons)
        .scale(0.6f)
        .parent(usernameLayout)
        .id("player-icon"_spr)
        .collect();

    auto& pcm = ProfileCacheManager::get();
    RichColor nameColor = util::ui::getNameRichColor(data.specialUserData);

    auto* nameButton = Build<CCLabelBMFont>::create(data.name.data(), "bigFont.fnt")
        .with([&nameColor](auto* label) {
            nameColor.animateLabel(label);
        })
        .limitLabelWidth(130.f, 0.45f, 0.1f)
        .intoMenuItem([this] {
            util::gd::openProfile(accountData.accountId, accountData.userId, accountData.name);
        })
        .scaleMult(1.1f)
        .parent(usernameLayout)
        .store(nameBtn)
        .collect();

    CCSprite* badgeIcon = util::ui::createBadgeIfSpecial(data.specialUserData);
    if (badgeIcon) {
        util::ui::rescaleToMatch(badgeIcon, util::ui::BADGE_SIZE);
        usernameLayout->addChild(badgeIcon);
    }

    if (isFriend) {
        // cell gradient
        auto* cellGradient = Build<CCSprite>::createSpriteName("friend-gradient.png"_spr)
            .color(globed::into<ccColor3B>(globed::color::FriendIngameGradient))
            .opacity(globed::color::FriendIngameGradient.a)
            .pos(0, 0)
            .anchorPoint({0, 0})
            .zOrder(-2)
            .scaleX(1.5)
            .blendFunc({GL_ONE, GL_ONE})
            .parent(this)
            .collect();

        util::ui::rescaleToMatch(cellGradient, {cellGradient->getScaledContentSize().width, CELL_HEIGHT}, true);

        // friend icon
        Build<CCSprite>::createSpriteName("friend-icon.png"_spr)
            .anchorPoint({0, 0.5})
            .scale(0.3)
            .parent(usernameLayout);
    }

    auto acm = GJAccountManager::get();
    if (data.accountId == acm->m_accountID) {
        CCSprite* gradient = Build<CCSprite>::createSpriteName("friend-gradient.png"_spr)
            .color(globed::into<ccColor3B>(globed::color::SelfIngameGradient))
            .opacity(globed::color::SelfIngameGradient.a)
            .pos(0, 0)
            .anchorPoint({0, 0})
            .zOrder(-2)
            .scaleX(10)
            .parent(this);

        util::ui::rescaleToMatch(gradient, {gradient->getScaledContentSize().width, CELL_HEIGHT}, true);
    }

    // percentage label
    Build<CCLabelBMFont>::create("", "goldFont.fnt")
        .scale(0.4f)
        .parent(usernameLayout)
        .id("percentage-label"_spr)
        .store(percentageLabel);

    usernameLayout->updateLayout();
    menu->updateLayout();

    this->fixNamePosition();

    this->makeButtons();

    this->refreshData(entry);
    this->schedule(schedule_selector(GlobedUserCell::updateVisualizer), 1.f / 60.f);

    return true;
}

void GlobedUserCell::refreshData(const PlayerStore::Entry& entry) {
    if (_data != entry) {
        _data = entry;

        bool platformer = GJBaseGameLayer::get()->m_level->isPlatformer();
        if (platformer && _data.localBest != 0) {
            percentageLabel->setString(util::format::formatPlatformerTime(_data.localBest).c_str());
            usernameLayout->updateLayout();
            this->fixNamePosition();
        } else if (!platformer) {
            percentageLabel->setString(fmt::format("{}%", _data.localBest).c_str());
            usernameLayout->updateLayout();
            this->fixNamePosition();
        }
    }
}

void GlobedUserCell::fixNamePosition() {
    nameBtn->setPositionY(CELL_HEIGHT / 2 - 2.5f);
}

void GlobedUserCell::updateVisualizer(float dt) {
#ifdef GLOBED_VOICE_SUPPORT
    if (audioVisualizer) {
        auto& vpm = VoicePlaybackManager::get();

        float loudness = vpm.getLoudness(accountData.accountId);
        audioVisualizer->setVolume(loudness);
    }
#endif // GLOBED_VOICE_SUPPORT
}

void GlobedUserCell::makeButtons() {
    if (buttonsWrapper) buttonsWrapper->removeFromParent();
    const float gap = 3.f;

    Build<CCMenu>::create()
        .anchorPoint(1.0f, 0.5f)
        .pos(GlobedUserListPopup::LIST_WIDTH - 7.f, CELL_HEIGHT / 2.f)
        .layout(RowLayout::create()
                    ->setGap(gap)
                    ->setAutoScale(false)
                    ->setAxisReverse(true))
        .parent(menu)
        .store(buttonsWrapper);

    auto& settings = GlobedSettings::get();
    auto pl = GlobedGJBGL::get();

    auto maxWidth = -5.f;

    bool notSelf = accountData.accountId != GJAccountManager::get()->m_accountID;
    bool createBtnSettings = notSelf;
    bool createBtnAdmin = AdminManager::get().authorized();
    bool createBtn2plink = pl->m_fields->roomSettings.flags.twoPlayerMode && notSelf;
    bool createBtnTp = createBtnAdmin && notSelf && !createBtn2plink;
    bool createVisualizer = settings.communication.voiceEnabled && notSelf;
    bool createSettingsAlts = false;

    if (!createBtnAdmin && createBtnSettings) {
        createSettingsAlts = true;
        createBtnSettings = false;
    }

    if (!pl->m_fields->players.contains(accountData.accountId)) {
        this->parent->removeListCell(this);
        return;
    }

    if (createBtnSettings) {
        // settings button
        Build<CCSprite>::createSpriteName("GJ_optionsBtn_001.png")
            .scale(0.36f)
            .intoMenuItem([this, id = accountData.accountId](auto) {
                auto* pl = GlobedGJBGL::get();

                // if they left the level, do nothing
                if (!pl->m_fields->players.contains(accountData.accountId)) {
                    this->parent->removeListCell(this);
                    return;
                }

                GlobedUserActionsPopup::create(id)->show();
            })
            .scaleMult(1.2f)
            .parent(buttonsWrapper)
            .id("player-actions-button"_spr)
            .store(actionsButton);

        maxWidth += actionsButton->getScaledContentSize().width + gap;
    } else if (createSettingsAlts) {
        bool isUnblocked = pl->shouldLetMessageThrough(accountData.accountId);
        bool isHidden = pl->m_fields->players.at(accountData.accountId)->getForciblyHidden();

        Build<CCSprite>::createSpriteName(isUnblocked ? "icon-mute.png"_spr : "icon-unmute.png"_spr)
            .scale(0.45f)
            .intoMenuItem([this, isUnblocked, accountId = accountData.accountId](auto) {
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

                this->makeButtons();
            })
            .scaleMult(1.2f)
            .parent(buttonsWrapper)
            .id("player-mute-button"_spr)
            .store(muteButton);

        maxWidth += muteButton->getScaledContentSize().width + gap;

        auto spr = Build<CCSprite>::createSpriteName(isHidden ? "icon-show-player.png"_spr : "icon-hide-player.png"_spr).collect();
        util::ui::rescaleToMatch(spr, muteButton);

        Build(spr)
            .intoMenuItem([this, isHidden, pl, accountId = accountData.accountId](auto) {
                if (!pl->m_fields->players.contains(accountId)) {
                    this->parent->removeListCell(this);
                    return;
                }

                pl->m_fields->players.at(accountId)->setForciblyHidden(!isHidden);
                auto& bl = BlockListManager::get();
                bl.setHidden(accountId, !isHidden);

                this->makeButtons();
            })
            .scaleMult(1.2f)
            .parent(buttonsWrapper)
            .id("player-hide-button"_spr)
            .store(hideButton);

        maxWidth += hideButton->getScaledContentSize().width + gap;
    }

    // admin menu button
    if (createBtnAdmin) {
        Build<CCSprite>::createSpriteName("GJ_reportBtn_001.png")
            .scale(0.4f)
            .intoMenuItem([this](auto) {
                AdminManager::get().openUserPopup(accountData.makeRoomPreview(0));
            })
            .parent(buttonsWrapper)
            .id("kick-button"_spr)
            .store(kickButton);

        maxWidth += kickButton->getScaledContentSize().width + gap;
    }

    if (createBtnTp) {
        Build<CCSprite>::createSpriteName("icon-teleport.png"_spr)
            .scale(0.35f)
            .intoMenuItem([pl, this](auto) {
                auto& settings = GlobedSettings::get();
                if (!settings.flags.seenTeleportNotice)  {
                    settings.flags.seenTeleportNotice = true;

                    FLAlertLayer::create(
                        "Note",
                        "Teleporting to a player will <cr>disable level progress</c> until you <cy>fully reset</c> the level.",
                        "Ok"
                    )->show();

                    return;
                }

                pl->toggleSafeMode(true);

                PlayerObject* po1 = pl->m_player1;
                CCPoint position = {0, 0}; // just in case the player has already left by the time we teleport
                if (pl->m_fields->interpolator->hasPlayer(accountData.accountId)) position = pl->m_fields->interpolator->getPlayerState(accountData.accountId).player1.position;

                po1->m_position = position;
            })
            .parent(buttonsWrapper)
            .id("teleport-button"_spr)
            .store(teleportButton);

        maxWidth += teleportButton->getScaledContentSize().width + gap;
    }

    if (createBtn2plink) {
        Build<CCSprite>::createSpriteName("gj_linkBtn_001.png")
            .scale(0.6f)
            .intoMenuItem([pl, this](auto) {
                pl->linkPlayerTo(this->accountData.accountId);
            })
            .parent(buttonsWrapper)
            .id("2p-link-button"_spr)
            .store(linkButton);

        maxWidth += linkButton->getScaledContentSize().width + gap;
    }

    if (createVisualizer) {
        // audio visualizer
        Build<GlobedAudioVisualizer>::create()
#ifndef GLOBED_VOICE_SUPPORT
            .visible(false)
#endif // GLOBED_VOICE_SUPPORT
            .parent(buttonsWrapper)
            .id("audio-visualizer"_spr)
            .store(audioVisualizer);

        audioVisualizer->setScaleX(0.5f);

        maxWidth += audioVisualizer->getScaledContentSize().width + gap;
    }

    buttonsWrapper->setContentSize({maxWidth, 20.f});

    buttonsWrapper->updateLayout();
}

GlobedUserCell* GlobedUserCell::create(const PlayerStore::Entry& entry, const PlayerAccountData& data, GlobedUserListPopup* parent) {
    auto ret = new GlobedUserCell;
    if (ret->init(entry, data, parent)) {
        return ret;
    }

    delete ret;
    return nullptr;
}