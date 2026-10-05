#pragma once

#include <globed/core/Module.hpp>

namespace globed {

class CollisionModule final : public SoftModule<CollisionModule> {
public:
    CollisionModule();

    static inline const ModuleMetadata metadata {
        .id = "globed.collision",
        .name = "Collision",
        .author = "Globed",
    };

    void onJoinLevel(GlobedGJBGL* gjbgl, GJGameLevel* level, bool editor) override;

    void checkCollisions(GlobedGJBGL* gjbgl, PlayerObject* player, float dt, bool p2);
};

}