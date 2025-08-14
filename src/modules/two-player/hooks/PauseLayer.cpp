#include <globed/config.hpp>
#include <globed/core/PopupManager.hpp>
#include <core/hooks/GJBaseGameLayer.hpp>
#include <modules/two-player/TwoPlayerModule.hpp>
#include <modules/ui/popups/UserListPopup.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedPauseLayer : geode::Modify<HookedPauseLayer, PauseLayer> {
    static void onModify(auto& self) {
        (void) self.setHookPriority("PauseLayer::onResume", -100000);

        GLOBED_CLAIM_HOOKS(TwoPlayerModule::get(), self,
            "PauseLayer::onResume",
        );
    }

    void onResume(CCObject* o) {
        auto& mod = TwoPlayerModule::get();
        if (!mod.isLinked()) {
            globed::quickPopup(
                "Globed Error",
                "To play in a <cy>2-player mode room</c>, you must <cg>Link</c> to another player. Open the user list menu?",
                "Dismiss", "Open",
                [](auto, bool open) {
                    if (!open) return;

                    UserListPopup::create()->show();
                }
            );

            return;
        }

        PauseLayer::onResume(o);
    }
};

}