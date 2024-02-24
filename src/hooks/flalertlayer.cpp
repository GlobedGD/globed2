#include "flalertlayer.hpp"

void HookedFLAlertLayer::keyBackClicked() {
    if (m_fields->blockClosing && util::time::now() < m_fields->blockClosingUntil) {
        // do nothing
    } else {
        FLAlertLayer::keyBackClicked();
    }
}