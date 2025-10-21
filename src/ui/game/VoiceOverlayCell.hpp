#pragma once

#include <globed/prelude.hpp>
#include <globed/core/data/PlayerDisplayData.hpp>
#include <ui/misc/AudioVisualizer.hpp>

namespace globed {

class VoiceOverlayCell : public CCNode {
public:
    static VoiceOverlayCell* create(const PlayerDisplayData& data);

    void updateLoudness(float loudness);

    inline int getAccountId() {
        return m_accountId;
    }

private:
    AudioVisualizer* m_visualizer;
    CCNode* m_wrapper;
    int m_accountId;

    bool init(const PlayerDisplayData& data);
};

}