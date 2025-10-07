#include "SendFeaturePopup.hpp"

using namespace geode::prelude;

namespace globed {

const CCSize SendFeaturePopup::POPUP_SIZE{ 340.f, 190.f };

bool SendFeaturePopup::setup(GJGameLevel* level) {
    m_level = level;
    return true;
}

}
