#include "PlayerListCell.hpp"
#include <core/net/NetworkManagerImpl.hpp>
#include <globed/core/FriendListManager.hpp>
#include <globed/core/RoomManager.hpp>
#include <globed/core/actions.hpp>
#include <globed/util/gd.hpp>
#include <globed/util/singleton.hpp>

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

bool PlayerListCell::init(int accountId, int userId, const std::string &username, const cue::Icons &icons,
                          const std::optional<SpecialUserData> &sud, CCSize cellSize)
{
    if (!CCNode::init())
        return false;

    this->setContentSize(cellSize);

    m_accountId = accountId;
    m_userId = userId;
    m_username = username;

    m_leftContainer =
        Build<CCNode>::create()
            .layout(RowLayout::create()->setAutoScale(false)->setAxisAlignment(AxisAlignment::Start)->setGap(3.f))
            .contentSize(cellSize.width - 20.f, cellSize.height)
            .pos(10.f, cellSize.height / 2.f)
            .anchorPoint(0.f, 0.5f)
            .parent(this);

    m_cubeIcon = Build(cue::PlayerIcon::create(icons)).id("icon").parent(m_leftContainer);
    m_cubeIcon->setLayoutOptions(AxisLayoutOptions::create()->setNextGap(5.f));

    cue::rescaleToMatch(m_cubeIcon, cellSize.height * 0.7f);

    m_nameLabel = Build(NameLabel::create(username, "bigFont.fnt"))
                      .with([&](auto lbl) {
                          lbl->setScale(cellSize.height * 0.65f / lbl->getContentHeight());
                          lbl->makeClickable([this, username = username](auto) {
                              globed::openUserProfile(m_accountId, m_userId, username);
                          });
                      })
                      .id("username-btn")
                      .parent(m_leftContainer)
                      .collect();

    m_nameLabel->setPositionY(m_nameLabel->getPositionY() + 1.f);

    if (sud) {
        m_nameLabel->updateWithRoles(*sud);
    }

    m_leftContainer->updateLayout();

    m_rightMenu = Build<CCMenu>::create()
                      .anchorPoint(1.f, 0.5f)
                      .pos(cellSize.width - 5.f, cellSize.height / 2.f)
                      .contentSize(cellSize.width - 10.f - 10.f, cellSize.height - 4.f)
                      .layout(RowLayout::create()
                                  ->setGap(5.f)
                                  ->setAxisAlignment(AxisAlignment::End)
                                  ->setAxisReverse(true)
                                  ->setAutoScale(false))
                      .parent(this);

    this->schedule(schedule_selector(PlayerListCell::updateStuff), 1.0f);

    this->customSetup();

    this->updateStuff(0.f);

    return true;
}

bool PlayerListCell::initMyself(cocos2d::CCSize cellSize)
{
    auto gam = cachedSingleton<GJAccountManager>();
    auto gm = cachedSingleton<GameManager>();

    auto sud = NetworkManagerImpl::get().getOwnSpecialData();

    return this->init(gam->m_accountID, gm->m_playerUserID, gam->m_username, getPlayerIcons(), sud, cellSize);
}

void PlayerListCell::updateStuff(float dt)
{
    auto &rm = RoomManager::get();

    if (auto teamId = rm.getTeamIdForPlayer(m_accountId)) {
        if (auto team = rm.getTeam(*teamId)) {
            m_nameLabel->updateTeam(*teamId, team->color);
            this->updateLayout();
        }
    }
}

void PlayerListCell::setGradient(CellGradientType type, bool blend)
{
    cue::resetNode(m_gradient);
    m_gradient = globed::addCellGradient(this, type, blend);
}

void PlayerListCell::initGradients(Context ctx)
{
    cue::resetNode(m_gradient);
    cue::resetNode(m_crownIcon);
    cue::resetNode(m_friendIcon);

    auto &rm = RoomManager::get();
    float iconSize = 16.f;

    if (m_accountId == globed::cachedSingleton<GJAccountManager>()->m_accountID) {
        this->setGradient(ctx == Context::Ingame ? CellGradientType::SelfIngame : CellGradientType::Self);
    }

    if (rm.isInRoom() && rm.getRoomOwner() == m_accountId) {
        if (!m_gradient)
            this->setGradient(CellGradientType::RoomOwner);
        m_crownIcon = Build<CCSprite>::create("icon-crown-small.png"_spr)
                          .with([&](auto spr) { cue::rescaleToMatch(spr, iconSize); })
                          .zOrder(3)
                          .parent(m_leftContainer);
    }

    if (FriendListManager::get().isFriend(m_accountId)) {
        if (!m_gradient)
            this->setGradient(ctx == Context::Ingame ? CellGradientType::FriendIngame : CellGradientType::Friend,
                              ctx == Context::Invites);
        m_friendIcon = Build<CCSprite>::create("icon-friend.png"_spr)
                           .with([&](auto spr) { cue::rescaleToMatch(spr, iconSize); })
                           .parent(m_leftContainer);
    }

    m_leftContainer->updateLayout();
}

PlayerListCell *PlayerListCell::create(int accountId, int userId, const std::string &username, const cue::Icons &icons,
                                       const std::optional<SpecialUserData> &sud, CCSize cellSize)
{
    auto ret = new PlayerListCell();
    if (ret->init(accountId, userId, username, icons, sud, cellSize)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

PlayerListCell *PlayerListCell::createMyself(cocos2d::CCSize cellSize)
{
    auto ret = new PlayerListCell();
    if (ret->initMyself(cellSize)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

} // namespace globed
