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

    bool init() override;

    void updateStream(int id, bool starving, float volume, float loudness);
};

}