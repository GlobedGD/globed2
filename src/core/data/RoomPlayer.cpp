#include <globed/core/data/RoomPlayer.hpp>
#include <cue/PlayerIcon.hpp>

cue::PlayerIcon* globed::RoomPlayer::createIcon() const {
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
