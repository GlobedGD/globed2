#include <modules/two-player/TwoPlayerModule.hpp>
#include <globed/config.hpp>
#include <globed/core/game/VisualPlayer.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR TPPlayerObject : geode::Modify<TPPlayerObject, PlayerObject> {
    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(TwoPlayerModule::get(), self,
            "PlayerObject::update"
        );
    }

    $override
    void update(float dt) {
        PlayerObject::update(dt);
    }
};

}