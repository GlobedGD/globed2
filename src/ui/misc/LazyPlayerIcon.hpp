#pragma once

#include <cue/PlayerIcon.hpp>

namespace globed {

/// A wrapper over a cue::PlayerIcon (which itself is a SimplePlayer)
/// that asynchronously loads the icon textures to avoid lagspikes.
class LazyPlayerIcon : public cue::PlayerIcon {
public:
    using Icons = cue::Icons;

    static LazyPlayerIcon* create(const Icons& icons);

    inline static PlayerIcon* create(IconType type, int id = 1, int color1 = 1, int color2 = 3, int glowColor = -1) {
        return create(Icons{type, id, color1, color2, glowColor});
    }

    void updateIcons(const Icons& icons);

protected:
    Icons m_realIcons;
    bool m_waiting = false;

    bool init(const Icons& icons);
    void onLoaded();
};

}
