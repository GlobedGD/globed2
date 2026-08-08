#include "PlayerListCell.hpp"
#include <globed/core/actions.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/FriendListManager.hpp>
#include <globed/util/singleton.hpp>
#include <globed/util/gd.hpp>
#include <core/net/NetworkManagerImpl.hpp>

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

bool PlayerListCell::init(
    int accountId,
    int userId,
    const std::string& username,
    const cue::Icons& icons,
    const std::optional<SpecialUserData>& sud,
    CCSize cellSize
) {
    if (!CCNode::init()) return false;

    this->setContentSize(cellSize);

    m_accountId = accountId;
    m_userId = userId;
    m_username = username;

    m_leftContainer = Build<CCNode>::create()
        .layout(SimpleRowLayout::create()
            ->setMainAxisScaling(AxisScaling::ScaleDown)
            ->setMainAxisAlignment(MainAxisAlignment::Start)
            ->setGap(3.f)
        )
        .contentSize(cellSize.width * 0.62f, cellSize.height)
        .pos(8.f, cellSize.height / 2.f)
        .anchorPoint(0.f, 0.5f)
        .id("left-container")
        .parent(this);

    m_cubeIcon = Build(LazyPlayerIcon::create(icons))
        .id("icon")
        .parent(m_leftContainer);
    m_cubeIcon->setLayoutOptions(SimpleAxisLayoutOptions::create()->setScalingPriority(ScalingPriority::Never));

    Build<AxisGap>::create(4.f)
        .parent(m_leftContainer);

    cue::rescaleToMatch(m_cubeIcon, cellSize.height * 0.7f);

    m_nameLabel = Build(NameLabel::create(username, "bigFont.fnt"))
        .with([&](auto lbl) {
            // reimpl limitLabelWidth because NameLabel shenanigans, we want name to be max 130.0 units wide
            float targetScale = cellSize.height * 0.65f / lbl->getContentHeight();
            float scale = std::clamp(targetScale, 0.6f, 130.f / lbl->getContentWidth());
            lbl->setScale(scale);

            lbl->makeClickable(
                [this, username = username](auto) {
                   globed::openUserProfile(m_accountId, m_userId, username);
                }
            );
        })
        .id("username-btn")
        .parent(m_leftContainer)
        .collect();

    m_nameLabel->setPositionY(m_nameLabel->getPositionY() + 1.f);
    m_nameLabel->setMultipleBadges(true);

    if (sud) {
        m_nameLabel->updateWithRoles(*sud);
    }

    m_leftContainer->updateLayout();

    m_rightMenu = Build<CCMenu>::create()
        .anchorPoint(1.f, 0.5f)
        .pos(cellSize.width - 5.f, cellSize.height / 2.f)
        .contentSize(cellSize.width * 0.34f, cellSize.height - 4.f)
        .layout(SimpleRowLayout::create()->setGap(5.f)
            ->setMainAxisAlignment(MainAxisAlignment::Start)
            ->setMainAxisDirection(AxisDirection::RightToLeft)
            ->setMainAxisScaling(AxisScaling::ScaleDown)
        )
        .parent(this);

    this->schedule(schedule_selector(PlayerListCell::updateStuff), 1.0f);

    this->customSetup();

    this->updateStuff(0.f);

    return true;
}

bool PlayerListCell::initMyself(cocos2d::CCSize cellSize) {
    auto gam = singleton<GJAccountManager>();
    auto gm = singleton<GameManager>();

    auto sud = NetworkManagerImpl::get().getOwnSpecialData();

    return this->init(
        gam->m_accountID,
        gm->m_playerUserID,
        gam->m_username,
        getPlayerIcons(),
        sud,
        cellSize
    );
}

void PlayerListCell::updateStuff(float dt) {
    auto& rm = RoomManager::get();

    if (rm.getSettings().teams) {
        if (auto teamId = rm.getTeamIdForPlayer(m_accountId)) {
            if (auto team = rm.getTeam(*teamId)) {
                m_nameLabel->updateTeam(*teamId, team->color);
                this->updateLayout();
            }
        }
    } else {
        m_nameLabel->updateNoTeam();
    }

    this->updateLayout();
}

void PlayerListCell::setGradient(CellGradientType type, bool blend) {
    cue::resetNode(m_gradient);
    m_gradient = globed::addCellGradient(this, type, blend);
}

void PlayerListCell::initGradients(Context ctx) {
    cue::resetNode(m_gradient);
    cue::resetNode(m_crownIcon);
    cue::resetNode(m_friendIcon);

    auto& rm = RoomManager::get();
    float iconSize = 16.f;

    if (m_accountId == globed::singleton<GJAccountManager>()->m_accountID) {
        this->setGradient(ctx == Context::Ingame ? CellGradientType::SelfIngame : CellGradientType::Self);
    }

    if (rm.isInRoom() && rm.getRoomOwner() == m_accountId) {
        if (!m_gradient) this->setGradient(CellGradientType::RoomOwner);
        m_crownIcon = Build<CCSprite>::create("icon-crown-small.png"_spr)
            .with([&](auto spr) { cue::rescaleToMatch(spr, iconSize); })
            .zOrder(3)
            .parent(m_leftContainer);
    }

    if (FriendListManager::get().isFriend(m_accountId)) {
        if (!m_gradient) this->setGradient(ctx == Context::Ingame ? CellGradientType::FriendIngame : CellGradientType::Friend, ctx == Context::Invites);
        m_friendIcon = Build<CCSprite>::create("icon-friend.png"_spr)
            .with([&](auto spr) { cue::rescaleToMatch(spr, iconSize); })
            .parent(m_leftContainer);
    }

    m_leftContainer->updateLayout();
}

PlayerListCell* PlayerListCell::create(
    int accountId,
    int userId,
    const std::string& username,
    const cue::Icons& icons,
    const std::optional<SpecialUserData>& sud,
    CCSize cellSize
) {
    auto ret = new PlayerListCell();
    if (ret->init(accountId, userId, username, icons, sud, cellSize)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

PlayerListCell* PlayerListCell::createMyself(cocos2d::CCSize cellSize) {
    auto ret = new PlayerListCell();
    if (ret->initMyself(cellSize)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

}
