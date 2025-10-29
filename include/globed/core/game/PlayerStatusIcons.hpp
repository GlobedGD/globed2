#pragma once

#include <globed/prelude.hpp>
#include <Geode/Geode.hpp>

namespace globed {

struct PlayerStatusFlags {
    bool paused;
    bool practicing;
    bool speaking;
    bool speakingMuted;
    bool editing;
    float loudness;

    bool operator==(const PlayerStatusFlags& other) const = default;
};

class GLOBED_DLL PlayerStatusIcons : public CCNode {
public:
    PlayerStatusIcons() = default;
    GLOBED_NOCOPY(PlayerStatusIcons);
    GLOBED_NOMOVE(PlayerStatusIcons);

    void updateStatus(const PlayerStatusFlags& flags, bool force = false);
    void setOpacity(unsigned char opacity);

    static PlayerStatusIcons* create(unsigned char opacity);

private:
    CCNode* m_iconWrapper = nullptr;
    PlayerStatusFlags m_flags;
    float m_nameScale = 0.f;
    unsigned char m_opacity = 255;

    bool init(unsigned char opacity);
};

}