#include "end_level_layer.hpp"

#ifndef GLOBED_LESS_BINDINGS

#include "gjbasegamelayer.hpp"

using namespace geode::prelude;

void HookedEndLevelLayer::customSetup() {
    EndLevelLayer::customSetup();

    auto* messageLabel = static_cast<TextArea*>(this->m_mainLayer->getChildByID("complete-message"));
    auto* endText = static_cast<CCLabelBMFont*>(this->m_mainLayer->getChildByID("end-text"));

    if (GlobedGJBGL::get()->isSafeMode()) {
        if (messageLabel) {
            messageLabel->setString(globed::string<"safe-mode-message">());
        }

        if (endText) {
            endText->setString(globed::string<"safe-mode-message">());
            endText->limitLabelWidth(150.0,0.8,0.0);
        }

    }
}

#endif // GLOBED_LESS_BINDINGS
