#include "PlayerListCell.hpp"
#include <globed/core/actions.hpp>

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

    this->setLayout(RowLayout::create()->setAutoScale(false)->setAxisAlignment(AxisAlignment::Start));

    m_cubeIcon = Build(cue::PlayerIcon::create(icons))
        .id("icon")
        .parent(this);

    cue::rescaleToMatch(m_cubeIcon, cellSize.height * 0.75f);

    m_usernameBtn = Build<CCLabelBMFont>::create(username.c_str(), "bigFont.fnt")
        .scale(cellSize.height / 48.f)
        .intoMenuItem([this, username = username](auto) {
            globed::openUserProfile(m_accountId, m_userId, username);
        })
        .scaleMult(1.17f)
        .id("username-btn")
        .parent(this);

    this->setContentSize({cellSize.width - 20.f, cellSize.height});
    this->ignoreAnchorPointForPosition(false);
    this->updateLayout();

    return true;
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

}
