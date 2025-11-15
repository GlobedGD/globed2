#pragma once

#include <globed/prelude.hpp>

#include <Geode/Geode.hpp>
#include <cue/LoadingCircle.hpp>

namespace globed {

class PinnedLevelCell : public CCNode {
public:
    static constexpr float HEIGHT = 50.f;

    static PinnedLevelCell* create(float width);

    void loadLevel(int id);

    inline void setUpdateCallback(std23::move_only_function<void()> callback) {
        m_updateCallback = std::move(callback);
    }

private:
    Ref<GJGameLevel> m_level;
    LevelCell* m_levelCell = nullptr;
    cue::LoadingCircle* m_circle = nullptr;
    std23::move_only_function<void()> m_updateCallback;
    bool m_collapsed = false;

    bool init(float width);

    inline void invokeCallback() {
        if (m_updateCallback) {
            m_updateCallback();
        }
    }

    void onLevelLoaded(GJGameLevel* level);
};

}