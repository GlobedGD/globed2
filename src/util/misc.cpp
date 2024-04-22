#include "misc.hpp"
#include <data/types/game.hpp>
#include <data/types/gd.hpp>

#include <asp/sync.hpp>
#include <util/simd.hpp>
#include <util/lowlevel.hpp>

namespace util::misc {
    bool swapFlag(bool& target) {
        bool state = target;
        target = false;
        return state;
    }

    // IconType -> PlayerIconType
    template<> PlayerIconType convertEnum<PlayerIconType, IconType>(IconType value) {
        switch (value) {
            case IconType::Cube: return PlayerIconType::Cube;
            case IconType::Ship: return PlayerIconType::Ship;
            case IconType::Ball: return PlayerIconType::Ball;
            case IconType::Ufo: return PlayerIconType::Ufo;
            case IconType::Wave: return PlayerIconType::Wave;
            case IconType::Robot: return PlayerIconType::Robot;
            case IconType::Spider: return PlayerIconType::Spider;
            case IconType::Swing: return PlayerIconType::Swing;
            case IconType::Jetpack: return PlayerIconType::Jetpack;
            default: return PlayerIconType::Cube;
        }
    }

    // PlayerIconType -> IconType
    template<> IconType convertEnum<IconType, PlayerIconType>(PlayerIconType value) {
        switch (value) {
            case PlayerIconType::Cube: return IconType::Cube;
            case PlayerIconType::Ship: return IconType::Ship;
            case PlayerIconType::Ball: return IconType::Ball;
            case PlayerIconType::Ufo: return IconType::Ufo;
            case PlayerIconType::Wave: return IconType::Wave;
            case PlayerIconType::Robot: return IconType::Robot;
            case PlayerIconType::Spider: return IconType::Spider;
            case PlayerIconType::Swing: return IconType::Swing;
            case PlayerIconType::Jetpack: return IconType::Jetpack;
            default: return IconType::Cube;
        }
    }

    void callOnce(const char* key, std::function<void()> func) {
        static std::unordered_set<const char*> called;

        if (called.contains(key)) {
            return;
        }

        called.insert(key);
        func();
    }

    void callOnceSync(const char* key, std::function<void()> func) {
        static asp::Mutex<> mtx;

        auto guard = mtx.lock();
        callOnce(key, func);
    }

    float calculatePcmVolume(const float* pcm, size_t samples) {
        return simd::calcPcmVolume(pcm, samples);
    }

    float pcmVolumeSlow(const float* pcm, size_t samples) {
        double sum = 0.0f;
        for (size_t i = 0; i < samples; i++) {
            sum += static_cast<double>(std::abs(pcm[i]));
        }

        return static_cast<float>(sum / static_cast<double>(samples));
    }

    bool compareName(const std::string_view nv1, const std::string_view nv2) {
        std::string name1(nv1);
        std::string name2(nv2);

        std::transform(name1.begin(), name1.end(), name1.begin(), ::tolower);
        std::transform(name2.begin(), name2.end(), name2.begin(), ::tolower);

        return name1 < name2;
    }

    bool isEditorCollabLevel(LevelId levelId) {
        const LevelId bound = std::pow(2, 32);

        return levelId > bound;
    }

    int getIconWithType(const PlayerIconData& data, PlayerIconType type) {
        int newIcon = data.cube;

        switch (type) {
            case PlayerIconType::Cube: newIcon = data.cube; break;
            case PlayerIconType::Ship: newIcon = data.ship; break;
            case PlayerIconType::Ball: newIcon = data.ball; break;
            case PlayerIconType::Ufo: newIcon = data.ufo; break;
            case PlayerIconType::Wave: newIcon = data.wave; break;
            case PlayerIconType::Robot: newIcon = data.robot; break;
            case PlayerIconType::Spider: newIcon = data.spider; break;
            case PlayerIconType::Swing: newIcon = data.swing; break;
            case PlayerIconType::Jetpack: newIcon = data.jetpack; break;
            default: newIcon = data.cube; break;
        };

        return newIcon;
    }

    int getIconWithType(const PlayerIconData& data, IconType type) {
        return getIconWithType(data, convertEnum<PlayerIconType>(type));
    }
}
