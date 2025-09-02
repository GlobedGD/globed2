#include "TeamManagementPopup.hpp"
#include <globed/core/PopupManager.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/util/color.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

const CCSize TeamManagementPopup::POPUP_SIZE = {300.f, 250.f};

static constexpr float CELL_HEIGHT = 26.f;
static constexpr float CELL_WIDTH = 260.f;

static ccColor4B getDefaultColor(size_t idx) {
    static constexpr ccColor4B colors[] = {
        colorFromHex("#ffffff"),
        colorFromHex("#fa3939"),
        colorFromHex("#00A2FF"),
        colorFromHex("#00FF00"),
        colorFromHex("#FFD300"),
        colorFromHex("#FF7F00"),
        colorFromHex("#8F00FF"),
        colorFromHex("#FF1493"),
        colorFromHex("#00FFFF"),
        colorFromHex("#228B22"),
        colorFromHex("#FF4500"),
        colorFromHex("#FF69B4"),
        colorFromHex("#9932CC"),
        colorFromHex("#00CED1"),
        colorFromHex("#B22222"),
        colorFromHex("#2E8B57"),
        colorFromHex("#F08080"),
        colorFromHex("#20B2AA"),
        colorFromHex("#4169E1"),
        colorFromHex("#708090")
    };

    size_t count = sizeof(colors) / sizeof(colors[0]);

    return colors[idx % count];
}

class TeamCell : public CCNode, public ColorPickPopupDelegate {
public:
    static TeamCell* create(const RoomTeam& team, size_t idx, TeamManagementPopup* popup) {
        auto ret = new TeamCell;
        if (ret->init(team, idx, popup)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

private:
    TeamManagementPopup* m_popup;
    ColorChannelSprite* m_colorSprite;
    CCMenuItemSpriteExtra* m_joinBtn = nullptr;
    CCNode* m_rightContainer;
    ccColor4B m_color;
    ccColor4B m_originalColor;
    size_t m_idx;

    bool init(const RoomTeam& team, size_t idx, TeamManagementPopup* popup) {
        CCNode::init();

        this->setContentSize({CELL_WIDTH, CELL_HEIGHT});

        bool editable = popup->m_showPlus;

        m_popup = popup;
        m_color = team.color;
        m_originalColor = m_color;
        m_idx = idx;

        auto leftContainer = Build<CCMenu>::create()
            .layout(RowLayout::create()->setAutoScale(false)->setAxisAlignment(AxisAlignment::Start))
            .contentSize(CELL_WIDTH * 0.7f, CELL_HEIGHT)
            .anchorPoint(0.f, 0.f)
            .ignoreAnchorPointForPos(false)
            .pos(6.f, 1.f)
            .parent(this)
            .collect();

        auto colorBtn = Build(ColorChannelSprite::create())
            .with([&](auto spr) { cue::rescaleToMatch(spr, CELL_HEIGHT * 0.8f); })
            .color(cue::into<ccColor3B>(team.color))
            .store(m_colorSprite)
            .intoMenuItem([this] {
                this->openEditColor();
            })
            .scaleMult(1.1f)
            .parent(leftContainer)
            .collect();

        colorBtn->setEnabled(editable);

        auto text = fmt::format("Team {}", idx + 1);

#ifdef GLOBED_MODULE_TCD_HNS
        if (idx == 0) {
            text = "Hiders";
        } else {
            text = "Seekers";
        }
#endif

        Build<CCLabelBMFont>::create(text.c_str(), "bigFont.fnt")
            .scale(0.6f)
            .parent(leftContainer);

        leftContainer->updateLayout();

        colorBtn->setPositionY(colorBtn->getPositionY() + 1.f);

        m_rightContainer = Build<CCMenu>::create()
            .layout(RowLayout::create()->setAutoScale(false)->setAxisAlignment(AxisAlignment::End))
            .contentSize(CELL_WIDTH * 0.3f, CELL_HEIGHT)
            .anchorPoint(1.f, 1.f)
            .pos(CELL_WIDTH - 6.f, CELL_HEIGHT - 1.f)
            .parent(this)
            .collect();

        if (editable) {
            Build<CCSprite>::createSpriteName("GJ_deleteSongBtn_001.png")
                .with([&](auto spr) { cue::rescaleToMatch(spr, CELL_HEIGHT * 0.8f); })
                .intoMenuItem([this] {
                    m_popup->deleteTeam(m_idx);
                })
                .zOrder(99)
                .parent(m_rightContainer);
        }

        m_rightContainer->updateLayout();

        auto& rm = RoomManager::get();
        bool myTeam = m_idx == rm.getCurrentTeamId();
        bool canJoin = !rm.getSettings().lockedTeams || rm.isOwner();

        // button to either join or leave team
        if (myTeam || canJoin) {
            this->recreateJoinButton(myTeam);
        }

        return true;
    }

    void openEditColor() {
        auto popup = ColorPickPopup::create(m_color);
        popup->setDelegate(this);
        popup->show();
    }

    void join() {
        auto& nm = NetworkManagerImpl::get();
        nm.sendAssignTeam(m_popup->m_assigningFor, m_idx);

        for (auto cell : m_popup->m_list->iterChecked<TeamCell>()) {
            cell->recreateJoinButton(cell == this);
        }
    }

    void recreateJoinButton(bool joined) {
        if (m_joinBtn) {
            m_joinBtn->removeFromParent();
        }

        m_joinBtn = Build<CCSprite>::createSpriteName(joined ? "GJ_selectSongOnBtn_001.png" : "GJ_playBtn2_001.png")
            .with([&](auto btn) { cue::rescaleToMatch(btn, CELL_HEIGHT * 0.85f); })
            .intoMenuItem([this, joined] {
                if (joined) return;

                this->join();
            })
            .parent(m_rightContainer);

        m_rightContainer->updateLayout();
    }

    void updateColor(ccColor4B const& color) override {
        m_color = color;
        m_colorSprite->setColor(cue::into<ccColor3B>(color));
        m_popup->updateTeamColor(m_idx, m_color);
    }
};

bool TeamManagementPopup::setup(int assigningFor) {
    this->setTitle("Team Management");
    m_showPlus = RoomManager::get().isOwner() && !assigningFor;
    m_assigningFor = assigningFor;

    m_list = Build(cue::ListNode::create(
        {CELL_WIDTH, 170.f},
        cue::Brown,
        cue::ListBorderStyle::Comments
    ))
        .pos(this->fromCenter(0.f, -2.f))
        .visible(false)
        .parent(m_mainLayer);

    m_list->setCellHeight(CELL_HEIGHT);
    m_list->setJustify(cue::Justify::Center);

    m_bottomContainer = Build<CCMenu>::create()
        .layout(RowLayout::create()->setAutoScale(false))
        .contentSize(CELL_WIDTH, 30.f)
        .pos(this->fromBottom(20.f))
        .visible(false)
        .parent(m_mainLayer);

    auto toggler = Build(CCMenuItemExt::createTogglerWithStandardSprites(0.7f, [this](auto toggler) {
        this->setLockedTeams(!toggler->isOn());
    }))
        .scale(0.9f)
        .parent(m_bottomContainer)
        .collect();

    toggler->toggle(RoomManager::get().getSettings().lockedTeams);

    Build<CCLabelBMFont>::create("Locked teams", "bigFont.fnt")
        .scale(0.4f)
        .parent(m_bottomContainer);

    m_bottomContainer->updateLayout();

    this->startLoading();
    NetworkManagerImpl::get().sendRoomStateCheck();

    return true;
}

void TeamManagementPopup::startLoading() {
    m_loadingCircle = Build(cue::LoadingCircle::create(true))
        .pos(this->center())
        .parent(m_mainLayer);

    m_list->setVisible(false);
    m_bottomContainer->setVisible(false);

    auto& nm = NetworkManagerImpl::get();

    m_stateListener = nm.listen<msg::RoomStateMessage>([this](const auto& msg) {
        this->onLoaded(msg.teams);
        return ListenerResult::Continue;
    });

    m_creationListener = nm.listen<msg::TeamCreationResultMessage>([this](const auto& msg) {
        this->onTeamCreated(msg.success, msg.teamCount);
        return ListenerResult::Continue;
    });

    m_settingsListener = nm.listen<msg::RoomSettingsUpdatedMessage>([this](const auto& msg) {
        this->stopLoad();
        return ListenerResult::Continue;
    });
}

void TeamManagementPopup::onLoaded(const std::vector<RoomTeam>& teams) {
    this->stopLoad();

    m_list->setAutoUpdate(false);
    m_list->clear();

    // populate the list
    size_t i = 0;
    for (auto& team : teams) {
        m_list->addCell(TeamCell::create(team, i++, this));
    }

    // add the plus button
    this->addPlusButton();

    m_list->setAutoUpdate(true);
    m_list->updateLayout();
    m_list->scrollToTop();
}

void TeamManagementPopup::onTeamCreated(bool success, uint16_t teamCount) {
    this->stopLoad();

    size_t currentCount = m_list->size() - m_showPlus; // not counting plus button

    if (success) {
        // verify if the team count is what we expect it to be
        if (teamCount == currentCount + 1) {
            // good, add new team
            if (m_showPlus) {
                m_list->removeCell(m_list->size() - 1); // remove the plus cell
            }

            m_list->addCell(TeamCell::create(RoomTeam {
                .color = getDefaultColor(teamCount - 1)
            }, teamCount - 1, this));

            this->addPlusButton();
        } else {
            // something went wrong, refresh everything
            log::warn("Received unexpected team count: {}, had {} in the list", teamCount, currentCount);
            this->startLoading();
            NetworkManagerImpl::get().sendRoomStateCheck();
        }
    } else {
        // failed!
        globed::alert("Error", "Failed to create new team");
    }
}

void TeamManagementPopup::addPlusButton() {
    if (!m_showPlus) return;

    auto plusBtn = Build<CCSprite>::createSpriteName("GJ_plusBtn_001.png")
        .with([&](auto btn) { cue::rescaleToMatch(btn, CCSize{CELL_HEIGHT, CELL_HEIGHT}); })
        .intoMenuItem([this] {
            this->createTeam();
        })
        .scaleMult(1.1f)
        .collect();

    auto menu = Build<CCMenu>::create()
        .contentSize(plusBtn->getScaledContentSize())
        .ignoreAnchorPointForPos(false)
        .intoNewChild(plusBtn)
        .pos(plusBtn->getScaledContentSize() / 2.f)
        .intoParent()
        .collect();

    m_list->addCell(menu);
}

void TeamManagementPopup::stopLoad() {
    m_loadingCircle->removeFromParent();
    m_loadingCircle = nullptr;
    m_stateListener.reset();
    m_creationListener.reset();
    m_settingsListener.reset();

    m_list->setVisible(true);
    m_bottomContainer->setVisible(true);
}

void TeamManagementPopup::createTeam() {
    this->startLoading();

    NetworkManagerImpl::get().sendCreateTeam(getDefaultColor(m_list->size() - 1));
}

void TeamManagementPopup::deleteTeam(uint16_t teamId) {
    this->startLoading();

    auto& nm = NetworkManagerImpl::get();
    nm.sendDeleteTeam(teamId);
    nm.sendRoomStateCheck();
}

void TeamManagementPopup::updateTeamColor(uint16_t teamId, cocos2d::ccColor4B color) {
    NetworkManagerImpl::get().sendUpdateTeam(teamId, color);
}

void TeamManagementPopup::setLockedTeams(bool locked) {
    this->startLoading();

    auto settings = RoomManager::get().getSettings();
    settings.lockedTeams = locked;

    auto& nm = NetworkManagerImpl::get();
    nm.sendUpdateRoomSettings(settings);
}

}