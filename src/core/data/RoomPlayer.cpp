#include <globed/core/data/RoomPlayer.hpp>
#include <globed/util/singleton.hpp>
#include <cue/PlayerIcon.hpp>

namespace globed {

cue::PlayerIcon* MinimalRoomPlayer::createIcon() const {
    return cue::PlayerIcon::create(this->toIcons());
}

cue::Icons MinimalRoomPlayer::toIcons() const {
    return cue::Icons{
        .type = IconType::Cube,
        .id = this->cube,
        .color1 = this->color1.asIdx(),
        .color2 = this->color2.asIdx(),
        .glowColor = this->glowColor.asIdx(),
    };
}

MinimalRoomPlayer MinimalRoomPlayer::createMyself() {
    auto gm = cachedSingleton<GameManager>();
    auto gam = cachedSingleton<GJAccountManager>();

    MinimalRoomPlayer out{};
    out.accountData = {
        .accountId = gam->m_accountID,
        .userId = gm->m_playerUserID,
        .username = gam->m_username,
    };
    out.cube = gm->m_playerFrame;
    out.color1 = gm->m_playerColor;
    out.color2 = gm->m_playerColor2;
    if (gm->m_playerGlow) {
        out.glowColor = gm->m_playerGlowColor;
    }

    return out;
}

RoomPlayer RoomPlayer::createMyself() {
    RoomPlayer self{MinimalRoomPlayer::createMyself()};
    self.session = SessionId{};
    self.teamId = 0;
    return self;
}

}
