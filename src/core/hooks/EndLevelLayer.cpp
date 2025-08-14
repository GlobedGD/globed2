#include "GJBaseGameLayer.hpp"
#include <Geode/modify/EndLevelLayer.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedEndLevelLayer : geode::Modify<HookedEndLevelLayer, EndLevelLayer> {
    $override
    void customSetup() {
        EndLevelLayer::customSetup();

        auto* messageLabel = static_cast<TextArea*>(this->m_mainLayer->getChildByID("complete-message"));
        auto* endText = static_cast<CCLabelBMFont*>(this->m_mainLayer->getChildByID("end-text"));

        if (GlobedGJBGL::get()->isSafeMode()) {
            const char* msg = "Globed Safe Mode!";

            if (messageLabel) {
                messageLabel->setString(msg);
            }

            if (endText) {
                endText->setString(msg);
                endText->limitLabelWidth(150.0, 0.8, 0.0);
            }
        }
    }
};

}
