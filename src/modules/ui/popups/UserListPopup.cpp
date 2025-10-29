#include "UserListPopup.hpp"
#include <globed/core/ValueManager.hpp>
#include <globed/core/FriendListManager.hpp>
#include <globed/core/PlayerCacheManager.hpp>
#include <globed/audio/AudioManager.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <core/CoreImpl.hpp>
#include <ui/misc/PlayerListCell.hpp>
#include <ui/misc/AudioVisualizer.hpp>
#include <ui/misc/Sliders.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize UserListPopup::POPUP_SIZE {400.f, 280.f};
static constexpr CCSize LIST_SIZE = {340.f, 190.f};
static constexpr float CELL_HEIGHT = 27.f;
static constexpr CCSize CELL_SIZE{LIST_SIZE.width, CELL_HEIGHT};

constexpr static size_t countBools(auto&&... bools) {
    return (static_cast<size_t>(bools) + ... + 0);
}

namespace {

class PlayerCell : public PlayerListCell {
public:
    using PlayerListCell::m_accountId;
    using PlayerListCell::m_username;
    UserListPopup* m_popup;

    static PlayerListCell* create(
        int accountId,
        int userId,
        const std::string& username,
        const cue::Icons& icons,
        const std::optional<SpecialUserData>& sud,
        UserListPopup* popup
    ) {
        auto ret = new PlayerCell();
        ret->m_popup = popup;

        if (ret->init(accountId, userId, username, icons, sud, CELL_SIZE)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    static PlayerListCell* createMyself(UserListPopup* popup) {
        auto ret = new PlayerCell();
        ret->m_popup = popup;

        if (ret->initMyself(CELL_SIZE)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

protected:
    AudioVisualizer* m_visualizer = nullptr;

    bool customSetup() {
        auto gjbgl = GlobedGJBGL::get();

        // add buttons
        bool self = m_accountId == cachedSingleton<GJAccountManager>()->m_accountID;

        bool createBtnHide = !self;
        bool createBtnMute = !self;
        bool createBtnAdmin = NetworkManagerImpl::get().isAuthorizedModerator();
        bool createBtnTp = createBtnAdmin && !self;
        bool createVisualizer = !self && globed::setting<bool>("core.audio.voice-chat-enabled");
        size_t buttonCount = countBools(createBtnHide, createBtnMute, createBtnAdmin, createBtnTp, createVisualizer);

        // if no visualizer, max button count is 4, otherwise 2
        size_t maxButtonCount = createVisualizer ? 2 : 4;

        // if the buttons don't fit, create a settings button which shows a popup with the rest of the buttons
        bool createSettingsBtn = buttonCount > maxButtonCount;

        auto mainButtons = CCArray::create();
        auto popupButtons = CCArray::create();
        CCSize btnSizeSmall = {20.f, 20.f};
        CCSize btnSizeBig = {28.f, 28.f};
        CCSize btnSize = createSettingsBtn ? btnSizeBig : btnSizeSmall;

        // Create various buttons

        // god i hate this
        bool muteAndHideInCell = (!createVisualizer || buttonCount == 2);

        bool isMuted = !gjbgl->shouldLetMessageThrough(m_accountId);
        bool isHidden = self ? false : SettingsManager::get().isPlayerHidden(m_accountId);

        // Mute button

        auto muteOn = CCSprite::create("icon-mute.png"_spr);
        auto muteOff = CCSprite::create("icon-unmute.png"_spr);
        cue::rescaleToMatch(muteOn, muteAndHideInCell ? btnSizeSmall : btnSizeBig);
        cue::rescaleToMatch(muteOff, muteAndHideInCell ? btnSizeSmall : btnSize);

        auto muteButton = CCMenuItemExt::createToggler(
            muteOn,
            muteOff,
            [accountId = m_accountId, gjbgl](CCMenuItemToggler* btn) {
                bool muted = btn->isOn();
                auto& sm = SettingsManager::get();

                muted ? (sm.blacklistPlayer(accountId)) : (sm.whitelistPlayer(accountId));

                auto& am = AudioManager::get();
                if (muted) {
                    am.setStreamVolume(accountId, 0.f);
                } else {
                    am.setStreamVolume(accountId, gjbgl->calculateVolumeFor(accountId));
                }
        });

        muteButton->setID("mute-btn"_spr);
        muteButton->m_onButton->m_scaleMultiplier = 1.2f;
        muteButton->m_offButton->m_scaleMultiplier = 1.2f;
        muteButton->toggle(!isMuted); // fucked up

        // Hide button

        auto hideOn = CCSprite::create("icon-hide-player.png"_spr);
        auto hideOff = CCSprite::create("icon-show-player.png"_spr);
        cue::rescaleToMatch(hideOn, muteAndHideInCell ? btnSizeSmall : btnSizeBig);
        cue::rescaleToMatch(hideOff, muteAndHideInCell ? btnSizeSmall : btnSizeBig);

        auto hideButton = CCMenuItemExt::createToggler(
            hideOn,
            hideOff,
            [accountId = m_accountId, gjbgl](CCMenuItemToggler* btn) {
                bool hidden = btn->isOn();
                auto& sm = SettingsManager::get();
                sm.setPlayerHidden(accountId, hidden);

                auto player = gjbgl->getPlayer(accountId);
                if (player) {
                    player->setForceHide(hidden);
                }
        });

        hideButton->setID("hide-btn"_spr);
        hideButton->m_onButton->m_scaleMultiplier = 1.2f;
        hideButton->m_offButton->m_scaleMultiplier = 1.2f;
        hideButton->toggle(!isHidden); // fucked up

        if (createSettingsBtn) {
            popupButtons->addObject(muteButton);
            popupButtons->addObject(hideButton);
        } else {
            mainButtons->addObject(muteButton);
            mainButtons->addObject(hideButton);
        }

        // admin menu button
        if (createBtnAdmin) {
            auto btn = Build<CCSprite>::createSpriteName("GJ_reportBtn_001.png")
                .with([&](CCSprite* spr) {
                    cue::rescaleToMatch(spr, btnSizeSmall);
                })
                .intoMenuItem([accountId = m_accountId](auto) {
                    globed::openModPanel(accountId);
                })
                .id("admin-button"_spr)
                .collect();

            mainButtons->addObject(btn);
        }

        if (createBtnTp) {
            auto btn = Build<CCSprite>::create("icon-teleport.png"_spr)
                .with([&](CCSprite* spr) {
                    cue::rescaleToMatch(spr, btnSize);
                })
                .intoMenuItem([accountId = m_accountId, gjbgl](auto) {
                    if (!globed::swapFlag("core.flags.seen-teleport-notice")) {
                        globed::alert(
                            "Note",
                            "Teleporting to a player will <cr>disable level progress</c> until you <cy>fully reset</c> the level."
                        );
                        return;
                    }

                    gjbgl->toggleSafeMode();

                    if (auto player = gjbgl->getPlayer(accountId)) {
                        gjbgl->m_player1->m_position = player->player1()->getLastPosition();
                    } else {
                        globed::toastError("Player is not in the level");
                    }
                })
                .id("teleport-button"_spr)
                .collect();

            if (createSettingsBtn) {
                popupButtons->addObject(btn);
            } else {
                mainButtons->addObject(btn);
            }
        }

        if (createVisualizer) {
            // audio visualizer
            Build<AudioVisualizer>::create()
                .id("audio-visualizer"_spr)
                .zOrder(999999) // force to be on the left
                .store(m_visualizer);

            m_visualizer->setScaleX(0.5f);
            mainButtons->addObject(m_visualizer);
        }

        // add custom buttons
        CoreImpl::get().onUserlistSetup(
            m_rightMenu,
            m_accountId,
            self,
            m_popup
        );

        m_rightMenu->updateLayout();

        this->initGradients(Context::Ingame);

        return true;
    }
};

}


bool UserListPopup::setup() {
    this->setTitle("Players");

    m_noElasticity = true;

    m_list = Build(cue::ListNode::create(LIST_SIZE))
        .anchorPoint(0.5f, 1.f)
        .pos(this->fromTop(45.f))
        .parent(m_mainLayer);
    m_list->setCellColors(cue::DarkBrown, cue::Brown);
    m_list->setAutoUpdate(false);

    this->hardRefresh();

    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png")
        .scale(0.9f)
        .intoMenuItem([this](auto) {
            this->hardRefresh();
        })
        .pos(this->fromBottomRight(8.f, 8.f))
        .id("reload-btn"_spr)
        .parent(m_buttonMenu);

    Build<CCSprite>::createSpriteName("GJ_reportBtn_001.png")
        .scale(0.5f)
        .intoMenuItem(+[] {
            globed::confirmPopup(
                "Reporting",
                "Someone <cr>breaking</c> the <cl>Globed</c> rules?\n<cg>Join</c> the <cl>Globed</c> discord and <cr>report</c> them there!",
                "Cancel",
                "Join",
                +[](FLAlertLayer*) {
                    geode::utils::web::openLinkInBrowser(globed::constant<"discord">());
                }
            );
        })
        .pos(this->fromBottomLeft(20.f, 20.f))
        .id("report-btn"_spr)
        .parent(m_buttonMenu);

    // checkbox to toggle voice sorting
    auto* cbLayout = Build<CCMenu>::create()
        .layout(RowLayout::create()->setGap(5.f)->setAutoScale(false))
        .pos(this->fromBottom(20.f))
        .parent(m_mainLayer)
        .collect();

    m_volumeSlider = Build(createSlider())
        .scale(0.85f)
        .id("volume-slider");

    m_volumeSlider->setContentWidth(80.f);
    m_volumeSlider->setRange(0.0, 2.0);
    m_volumeSlider->setValue(globed::setting<float>("core.audio.playback-volume"));
    m_volumeSlider->setCallback([this](cue::Slider* slider, double value) {
        this->onVolumeChanged(value);
    });

    Build<CCLabelBMFont>::create("Voice Volume", "bigFont.fnt")
        .scale(0.45f * 0.7f)
        .intoNewParent(CCNode::create())
        .id("volume-wrapper")
        .child(m_volumeSlider)
        .layout(ColumnLayout::create()
            ->setAutoScale(false)
            ->setAxisReverse(true))
        .contentSize(80.f, 30.f)
        .anchorPoint(0.5f, 0.5f)
        .pos(this->fromTopRight(50.f, 25.f))
        .parent(m_mainLayer)
        .updateLayout();

    // Build<CCMenuItemToggler>(CCMenuItemToggler::createWithStandardSprites(this, menu_selector(GlobedUserListPopup::onToggleVoiceSort), 0.7f))
    //     .id("toggle-voice-sort"_spr)
    //     .parent(cbLayout);

    // Build<CCLabelBMFont>::create("Sort by voice", "bigFont.fnt")
    //     .scale(0.4f)
    //     .id("toggle-voice-sort-hint"_spr)
    //     .parent(cbLayout);

    cbLayout->updateLayout();

    // this->schedule(schedule_selector(GlobedUserListPopup::reorderWithVolume), 0.5f);

    return true;
}

void UserListPopup::hardRefresh() {
    m_list->clear();

    auto gjbgl = GlobedGJBGL::get();
    auto& players = gjbgl->m_fields->m_players;

    this->setTitle(fmt::format("Players ({})", players.size()));

    bool createdSelf = false;

    for (auto& [playerId, player] : players) {
        if (playerId == cachedSingleton<GJAccountManager>()->m_accountID) {
            m_list->addCell(PlayerCell::createMyself(this));
            createdSelf = true;
        } else if (player->isDataInitialized()) {
            auto& data = player->displayData();
            m_list->addCell(PlayerCell::create(
                data.accountId,
                data.userId,
                data.username,
                cue::Icons {
                    .type = IconType::Cube,
                    .id = data.icons.cube,
                    .color1 = data.icons.color1.asIdx(),
                    .color2 = data.icons.color2.asIdx(),
                    .glowColor = data.icons.glowColor.asIdx(),
                },
                data.specialUserData,
                this
            ));
        } else {
            log::warn("Uninitialized player, not adding to player list (ID {})", playerId);
        }
    }

    if (!createdSelf) {
        m_list->addCell(PlayerCell::createMyself(this));
        createdSelf = true;
    }

    auto selfId = cachedSingleton<GJAccountManager>()->m_accountID;
    auto& flm = FriendListManager::get();

    m_list->sortAs<PlayerCell>([&](PlayerCell* a, PlayerCell* b) {
        if (a->m_accountId == selfId) return true;
        else if (b->m_accountId == selfId) return false;

        bool aFriend = flm.isFriend(a->m_accountId);
        bool bFriend = flm.isFriend(b->m_accountId);

        if (aFriend != bFriend) {
            return aFriend;
        }

        auto aName = geode::utils::string::toLower(a->m_username);
        auto bName = geode::utils::string::toLower(b->m_username);

        return aName < bName;
    });

    m_list->updateLayout();
}

void UserListPopup::onVolumeChanged(double value) {
    globed::setting<float>("core.audio.playback-volume") = value;
}

}
