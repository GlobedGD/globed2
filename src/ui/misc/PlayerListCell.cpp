#include "PlayerListCell.hpp"
#include <globed/core/actions.hpp>
#include <globed/util/singleton.hpp>

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

bool PlayerListCell::init(
    int accountId,
    int userId,
    const std::string& username,
    const cue::Icons& icons,
    CCSize cellSize
) {
    if (!CCMenu::init()) return false;

    m_accountId = accountId;
    m_userId = userId;

    this->setLayout(
        RowLayout::create()
            ->setAutoScale(false)
            ->setAxisAlignment(AxisAlignment::Start)
            ->setGap(10.f)
    );

    m_cubeIcon = Build(cue::PlayerIcon::create(icons))
        .id("icon")
        .parent(this);

    cue::rescaleToMatch(m_cubeIcon, cellSize.height * 0.7f);

    m_usernameBtn = Build<CCLabelBMFont>::create(username.c_str(), "bigFont.fnt")
        .scale(cellSize.height / 52.f)
        .intoMenuItem([this, username = username](auto) {
            globed::openUserProfile(m_accountId, m_userId, username);
        })
        .scaleMult(1.1f)
        .id("username-btn")
        .parent(this);

    this->setContentSize({cellSize.width - 20.f, cellSize.height});
    this->ignoreAnchorPointForPosition(false);
    this->updateLayout();

    m_usernameBtn->setPositionY(m_usernameBtn->getPositionY() + 1.f);

    m_rightMenu = Build<CCMenu>::create()
        .anchorPoint(1.f, 0.5f)
        .pos(cellSize.width - 5.f, cellSize.height / 2.f)
        .contentSize(cellSize.width - 10.f - 10.f, cellSize.height - 4.f)
        .layout(RowLayout::create()->setGap(5.f)->setAxisAlignment(AxisAlignment::End)->setAxisReverse(true))
        .parent(this);

    this->customSetup();

    return true;
}

bool PlayerListCell::initMyself(cocos2d::CCSize cellSize) {
    auto gam = cachedSingleton<GJAccountManager>();
    auto gm = cachedSingleton<GameManager>();

    return this->init(
        gam->m_accountID,
        gm->m_playerUserID,
        gam->m_username,
        cue::Icons {
            .type = IconType::Cube,
            .id = gm->m_playerFrame,
            .color1 = gm->m_playerColor,
            .color2 = gm->m_playerColor2,
            .glowColor = gm->m_playerGlow ? gm->m_playerGlowColor : -1,
        },
        cellSize
    );
}

PlayerListCell* PlayerListCell::create(
    int accountId,
    int userId,
    const std::string& username,
    const cue::Icons& icons,
    CCSize cellSize
) {
    auto ret = new PlayerListCell();
    if (ret->init(accountId, userId, username, icons, cellSize)) {
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
