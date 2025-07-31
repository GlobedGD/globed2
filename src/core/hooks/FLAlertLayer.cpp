#include <globed/core/PopupManager.hpp>
#include <globed/config.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/FLAlertLayer.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR FLAlertLayerHook : geode::Modify<FLAlertLayerHook, FLAlertLayer> {
    void keyBackClicked() {
        auto& pm = PopupManager::get();
        if (!pm.isManaged(this)) {
            return FLAlertLayer::keyBackClicked();
        }

        auto pref = pm.manage(this);
        if (!pref.shouldPreventClosing()) {
            FLAlertLayer::keyBackClicked();
        }
    }
};

}
