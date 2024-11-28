#include "flalertlayer.hpp"

using namespace asp::time;

void HookedFLAlertLayer::keyBackClicked() {
    if (m_fields->blockClosing && m_fields->blockClosingUntil.isFuture()) {
        // do nothing
    } else {
        FLAlertLayer::keyBackClicked();
    }
}

void HookedFLAlertLayer::blockClosingFor(Duration duration) {
    m_fields->blockClosing = true;
    m_fields->blockClosingUntil = SystemTime::now() + duration;
}