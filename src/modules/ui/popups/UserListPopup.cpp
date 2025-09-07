#include "UserListPopup.hpp"
#include <globed/core/ValueManager.hpp>
#include <globed/core/FriendListManager.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <core/CoreImpl.hpp>
#include <ui/misc/PlayerListCell.hpp>
#include <ui/misc/Sliders.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize UserListPopup::POPUP_SIZE {400.f, 280.f};
static constexpr CCSize LIST_SIZE = {340.f, 190.f};
static constexpr float CELL_HEIGHT = 27.f;
static constexpr CCSize CELL_SIZE{LIST_SIZE.width, CELL_HEIGHT};

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
        UserListPopup* popup
    ) {
        auto ret = new PlayerCell();
        ret->m_popup = popup;

        if (ret->init(accountId, userId, username, icons, CELL_SIZE)) {
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
    bool customSetup() {
        // add buttons
        CoreImpl::get().onUserlistSetup(
            m_rightMenu,
            m_accountId,
            m_accountId == cachedSingleton<GJAccountManager>()->m_accountID,
            m_popup
        );

        m_rightMenu->updateLayout();

        return true;
    }
};

}


bool UserListPopup::setup() {
    this->setTitle("Players");

    m_noElasticity = true;

    m_list = Build(cue::ListNode::create(LIST_SIZE, cue::Brown, cue::ListBorderStyle::Comments))
        .anchorPoint(0.5f, 1.f)
        .pos(this->fromTop(45.f))
        .parent(m_mainLayer);
    m_list->setCellColors(cue::DarkBrown, cue::Brown);

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
        .intoMenuItem([this](auto) {
            createQuickPopup(
                "Reporting",
                "Someone <cr>breaking</c> the <cl>Globed</c> rules?\n<cg>Join</c> the <cl>Globed</c> discord and <cr>report</c> them there!",
                "Cancel",
                "Join",
                [this](auto, bool btn2) {
                    if (btn2) {
                        geode::utils::web::openLinkInBrowser(globed::constant<"discord">());
                    }
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
    // TODO
    // m_volumeSlider->setValue(GlobedSettings::get().communication.voiceVolume);
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
    m_list->setAutoUpdate(false);
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
            m_list->addCell(PlayerCell::create(data.accountId, data.userId, data.username, cue::Icons {
                .type = IconType::Cube,
                .id = data.icons.cube,
                .color1 = data.icons.color1.asIdx(),
                .color2 = data.icons.color2.asIdx(),
                .glowColor = data.icons.glowColor.asIdx(),
            }, this));
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

    m_list->setAutoUpdate(true);
    m_list->updateLayout();
    m_list->scrollToTop();
}

void UserListPopup::onVolumeChanged(double value) {
    // TODO: set volume
}

}
