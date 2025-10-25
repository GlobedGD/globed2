#pragma once

#include <Geode/Geode.hpp>
#include <globed/config.hpp>

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

class GLOBED_DLL PlayerStatusIcons : public cocos2d::CCNode {
public:
    void updateStatus(const PlayerStatusFlags& flags, bool force = false);

    static PlayerStatusIcons* create(unsigned char opacity);

private:
    cocos2d::CCNode* m_iconWrapper = nullptr;
    PlayerStatusFlags m_flags;
    float m_nameScale = 0.f;
    unsigned char m_opacity = 255;

    bool init(unsigned char opacity);
};

}