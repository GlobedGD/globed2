#include <Geode/modify/PlayerObject.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>

using namespace geode::prelude;

namespace globed { namespace {

struct GLOBED_NOVTABLE HookedPlayerObject : public Modify<HookedPlayerObject, PlayerObject> {
    $override
    void incrementJumps() {
        PlayerObject::incrementJumps();

        if (auto* gpl = GlobedGJBGL::get(m_gameLayer)) {
            if (this == gpl->m_player1) {
                gpl->recordPlayerJump(true);
            } else if (this == gpl->m_player2) {
                gpl->recordPlayerJump(false);
            }
        }
    }
};

}}
