#include <core/net/NetworkManagerImpl.hpp>
#include <globed/config.hpp>
#include <globed/core/RoomManager.hpp>
#include <modules/scripting-ui/ScriptingUIModule.hpp>
#include <modules/scripting-ui/ui/ScriptLogPanelPopup.hpp>
#include <modules/scripting/ScriptingModule.hpp>
#include <modules/scripting/hooks/GJBaseGameLayer.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <UIBuilder.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR SCPauseLayer : Modify<SCPauseLayer, PauseLayer> {
    static void onModify(auto &self)
    {
        GLOBED_CLAIM_HOOKS(ScriptingUIModule::get(), self, "PauseLayer::customSetup", );
    }

    $override void customSetup()
    {
        PauseLayer::customSetup();

        bool scriptingActive = false;
        bool scriptsSent = false;

        if (ScriptingModule::get().isEnabled()) {
            if (auto pl = SCBaseGameLayer::get()) {
                scriptingActive = pl->m_fields->m_hasScripts;
                scriptsSent = pl->m_fields->m_scriptsSent;
            }
        }

        // instead of the log window, show an alert if we are disconnected or not in a room
        auto &nm = NetworkManagerImpl::get();
        auto &rm = RoomManager::get();

        bool showAlert = scriptingActive && !scriptsSent && (!nm.isConnected() || rm.isInGlobal());

        if (!scriptingActive || (!showAlert && !nm.isConnected())) {
            return;
        }

        auto winSize = CCDirector::get()->getWinSize();

        auto menu = this->getChildByID("playerlist-menu"_spr);
        if (!menu) {
            log::error("playerlist-menu not found, cannot add logs button!");
            return;
        }

        Build<ButtonSprite>::create("Logs", "bigFont.fnt", "GJ_button_01.png", 0.7f)
            .scale(0.7f)
            .intoMenuItem([showAlert] {
                if (showAlert) {
                    globed::alert("Globed Error",
                                  "This level has <cj>script objects</c> inside, but you are either <cr>not "
                                  "connected</c> to the server or <cr>not in a room</c>.\n\n"
                                  "You <cr>must</c> be in a <cg>Globed room</c> in order for scripts to work.");
                } else {
                    ScriptLogPanelPopup::create()->show();
                }
            })
            .parent(menu);

        menu->updateLayout();
    }
};

} // namespace globed