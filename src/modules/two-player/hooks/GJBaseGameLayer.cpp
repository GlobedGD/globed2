#include <modules/two-player/TwoPlayerModule.hpp>
#include <globed/config.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

namespace globed {

struct GLOBED_MODIFY_ATTR TPMBaseGameLayer : geode::Modify<TPMBaseGameLayer, GJBaseGameLayer> {
    $override
    void updateCamera(float dt) {
        // TODO: stuff
        GJBaseGameLayer::updateCamera(dt);
    }
};

}