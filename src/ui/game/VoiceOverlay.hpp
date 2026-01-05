#pragma once

#include <globed/prelude.hpp>
#include "VoiceOverlayCell.hpp"

#include <Geode/Geode.hpp>

namespace globed {

class VoiceOverlay : public CCNode {
public:
    static VoiceOverlay* create();

    void update(float) override;
    void updateSoft();

private:
    std::unordered_map<int, VoiceOverlayCell*> m_cells;
    float m_threshold = 0.f;

    bool init() override;

    void updateStream(RemotePlayer& player, bool local);
    void updateStream(int id, bool starving, float loudness);
};

}