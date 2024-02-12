#include "user_cell.hpp"

#include "userlist.hpp"
#include <audio/voice_playback_manager.hpp>
#include <data/packets/client/admin.hpp>
#include <managers/block_list.hpp>
#include <managers/settings.hpp>
#include <hooks/play_layer.hpp>
#include <ui/general/ask_input_popup.hpp>
#include <util/format.hpp>

using namespace geode::prelude;

bool GlobedUserCell::init(const PlayerStore::Entry& entry, const PlayerAccountData& data) {
    if (!CCLayer::init()) return false;

    playerId = data.id;

    auto winSize = CCDirector::get()->getWinSize();

    auto gm = GameManager::get();

    auto sp = Build<SimplePlayer>::create(data.icons.cube)
        .playerFrame(data.icons.cube, IconType::Cube)
        .color(gm->colorForIdx(data.icons.color1))
        .secondColor(gm->colorForIdx(data.icons.color2))
        .parent(this)
        .pos(25.f, CELL_HEIGHT / 2.f)
        .id("player-icon"_spr)
        .collect();

    Build<CCMenu>::create()
        .pos(0.f, 0.f)
        .parent(this)
        .store(menu);

    auto* nameLabel = Build<CCLabelBMFont>::create(data.name.data(), "bigFont.fnt")
        .limitLabelWidth(150.f, 0.7f, 0.1f)
        .collect();

    auto* nameButton = Build<CCMenuItemSpriteExtra>::create(nameLabel, this, menu_selector(GlobedUserCell::onOpenProfile))
        // goodness
        .pos(sp->getPositionX() + nameLabel->getScaledContentSize().width / 2.f + 25.f, CELL_HEIGHT / 2.f)
        .parent(menu)
        .collect();

    if (data.icons.glowColor != -1) {
        sp->setGlowOutline(gm->colorForIdx(data.icons.glowColor));
    } else {
        sp->disableGlowOutline();
    }

    // percentage label
    Build<CCLabelBMFont>::create("", "goldFont.fnt")
        .pos(nameButton->getPosition() + nameButton->getScaledContentSize() / 2.f + CCPoint{3.f, -3.f})
        .anchorPoint({0.f, 0.5f})
        .scale(0.4f)
        .parent(this)
        .id("percentage-label"_spr)
        .store(percentageLabel);

    this->makeBlockButton();

    this->refreshData(entry);

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

void GlobedUserCell::makeBlockButton() {
    if (buttonsWrapper) buttonsWrapper->removeFromParent();
    Build<CCMenu>::create()
        .anchorPoint(0.f, 0.f)
        .pos(GlobedUserListPopup::LIST_WIDTH - 25.f, CELL_HEIGHT / 2.f)
        .layout(RowLayout::create()
                    ->setGap(5.f)
                    ->setAutoScale(false))
        .parent(menu)
        .store(buttonsWrapper);

    if (playerId != GJAccountManager::get()->m_accountID) {
        // mute/unmute button
        auto pl = static_cast<GlobedPlayLayer*>(PlayLayer::get());
        bool isUnblocked = pl->shouldLetMessageThrough(playerId);

        Build<CCSprite>::createSpriteName(isUnblocked ? "GJ_fxOnBtn_001.png" : "GJ_fxOffBtn_001.png")
            .scale(0.8f)
            .intoMenuItem([isUnblocked, this](auto) {
                auto& bl = BlockListMangaer::get();
                isUnblocked ? bl.blacklist(this->playerId) : bl.whitelist(this->playerId);
                // mute them immediately
                auto& settings = GlobedSettings::get();
                auto& vpm = VoicePlaybackManager::get();

                if (isUnblocked) {
                    vpm.setVolume(this->playerId, 0.f);
                } else {
                    vpm.setVolume(this->playerId, settings.communication.voiceVolume);
                }

                this->makeBlockButton();
            })
            .parent(buttonsWrapper)
            .id("block-button"_spr)
            .store(blockButton);
    }

    // kick button for admins
    if (NetworkManager::get().isAuthorizedAdmin()) {
        Build<CCSprite>::createSpriteName("accountBtn_blocked_001.png")
            .scale(0.7f)
            .intoMenuItem([this](auto) {
                AskInputPopup::create("Kick player", [this](const auto message) {
                    auto& nm = NetworkManager::get();
                    auto packet = AdminDisconnectPacket::create(std::to_string(this->playerId), message);
                    nm.send(packet);
                }, 64, "reason..", util::misc::STRING_PRINTABLE_INPUT)->show();
            })
            .parent(buttonsWrapper)
            .id("kick-button"_spr)
            .store(kickButton);
    }
}

void GlobedUserCell::onOpenProfile(CCObject*) {
    ProfilePage::create(playerId, playerId == GJAccountManager::get()->m_accountID)->show();
}

GlobedUserCell* GlobedUserCell::create(const PlayerStore::Entry& entry, const PlayerAccountData& data) {
    auto ret = new GlobedUserCell;
    if (ret->init(entry, data)) {
        return ret;
    }

    delete ret;
    return nullptr;
}