#include "end_level_layer.hpp"

#include "gjbasegamelayer.hpp"

using namespace geode::prelude;

void HookedEndLevelLayer::customSetup() {
    EndLevelLayer::customSetup();

    auto* messageLabel = static_cast<TextArea*>(this->m_mainLayer->getChildByID("complete-message"));
    auto* endText = static_cast<CCLabelBMFont*>(this->m_mainLayer->getChildByID("end-text"));

    if (GlobedGJBGL::get()->m_fields->shouldStopProgress) {
        if (messageLabel)
            messageLabel->setString("Safe Mode!");

        if (endText)
            endText->setString("Safe Mode!");
    }
}