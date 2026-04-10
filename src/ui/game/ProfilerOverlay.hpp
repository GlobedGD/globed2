#pragma once
#include <globed/prelude.hpp>

namespace globed {

struct ProfilerSample {
    asp::Duration time;
    geode::ZStringView name;
    cocos2d::ccColor4F color;

    ProfilerSample(geode::ZStringView name, asp::Instant start, asp::Instant end, std::string_view hex) {
        this->name = name;
        this->time = end.durationSince(start);

        auto col = geode::cocos::cc3bFromHexString(hex).unwrapOrDefault();
        this->color = cocos2d::ccColor4F {
            col.r / 255.f,
            col.g / 255.f,
            col.b / 255.f,
            1.0f
        };
    }
};

struct ProfilerFrame {
    asp::Duration totalTime;
    std::vector<ProfilerSample> samples;
};

class ProfilerOverlay : public CCNode {
public:
    static ProfilerOverlay* create(CCSize size);

    void updateWithFrame(const ProfilerFrame& frame);

private:
    cocos2d::CCDrawNode* m_drawNode;
    std::deque<std::pair<asp::Instant, ProfilerFrame>> m_frames;
    CCSize m_drawSize;
    CCNode* m_legend;
    std::unordered_set<std::string> m_legendNames;
    float m_smoothedMaxTime = 1.f / 60.f;

    bool init(CCSize size);

    void redraw();
    void addNewEntryToLegend(std::string_view name, cocos2d::ccColor4F color);
};

}
