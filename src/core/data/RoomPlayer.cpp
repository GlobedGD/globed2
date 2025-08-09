#include <globed/core/data/RoomPlayer.hpp>
#include <globed/util/singleton.hpp>
#include <cue/PlayerIcon.hpp>

namespace globed {

cue::PlayerIcon* RoomPlayer::createIcon() const {
    return cue::PlayerIcon::create(
        cue::Icons{
            .type = IconType::Cube,
            .id = this->cube,
            .color1 = this->color1,
            .color2 = this->color2,
            .glowColor = this->glowColor,
        }
    );
}

RoomPlayer RoomPlayer::createMyself() {
    auto gm = cachedSingleton<GameManager>();
    auto gam = cachedSingleton<GJAccountManager>();

    RoomPlayer out{};
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
    out.session = SessionId{};
    return out;
}

}
