#include "setup_instant_count_popup.hpp"

bool InstantCountPopupHook::init(CountTriggerGameObject* p0, cocos2d::CCArray* p1) {
    if (!SetupInstantCountPopup::init(p0, p1)) return false;

    return true;
}

void InstantCountPopupHook::updateItemID() {

}
