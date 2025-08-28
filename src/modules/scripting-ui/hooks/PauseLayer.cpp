#include <globed/config.hpp>
#include <modules/scripting-ui/ScriptingUIModule.hpp>
#include <modules/scripting-ui/ui/ScriptLogPanelPopup.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR SCPauseLayer : Modify<SCPauseLayer, PauseLayer> {
    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(ScriptingUIModule::get(), self,
            "PauseLayer::customSetup",
        );
    }

    $override
    void customSetup() {
        PauseLayer::customSetup();

        auto winSize = CCDirector::get()->getWinSize();

        auto menu = this->getChildByID("playerlist-menu"_spr);
        if (!menu) {
            log::error("playerlist-menu not found, cannot add logs button!");
            return;
        }

        Build<ButtonSprite>::create("Logs", "bigFont.fnt", "GJ_button_01.png", 0.7f)
            .scale(0.7f)
            .intoMenuItem([this] {
                ScriptLogPanelPopup::create()->show();
            })
            .parent(menu);

        menu->updateLayout();
    }
};


}