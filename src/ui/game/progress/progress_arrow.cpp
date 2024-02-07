#include "progress_arrow.hpp"

#include <util/math.hpp>

using namespace geode::prelude;

bool PlayerProgressArrow::init() {
    if (!CCNode::init()) return false;

    return true;
}

void PlayerProgressArrow::updatePosition(CCPoint cameraOrigin, CCSize cameraCoverage, CCPoint playerPosition) {
    // is the player visible? then dont show the arrow
    bool inCameraCoverage = (
        playerPosition.x >= cameraOrigin.x &&
        playerPosition.x <= cameraOrigin.x + cameraCoverage.width &&
        playerPosition.y >= cameraOrigin.y &&
        playerPosition.y <= cameraOrigin.y + cameraCoverage.height
    );

    if (inCameraCoverage) {
        // this->setVisible(false);
        // return;
    }

    this->setVisible(true);

    auto cameraCenter = cameraOrigin + cameraCoverage / 2;

    float dx = playerPosition.x - cameraCenter.x;
    float dy = playerPosition.y - cameraCenter.y;

    float angle = std::atan2(dy, dx);
    float angleDeg = angle * (180.f / M_PI);

    if (angleDeg < 0.f) {
        angleDeg += 360.f;
    }

    // right - 0 degrees
    // top - 90 degrees
    // left - 180 degrees
    // bottom - 270 degrees

    // log::debug("angle: {} degrees", angleDeg);

    // i'll be honest i do not understand any of this

    float edgeX, edgeY;

    float halfw = cameraCoverage.width / 2.0f;
    float halfh = cameraCoverage.height / 2.0f;
    float slope = std::tan(angle);

    if (util::math::equal(angle, 0) || util::math::equal(angle, (float)M_PI) || util::math::equal(angle, (float)-M_PI)) {
        // horizontal line
        edgeX = cameraOrigin.x + halfw;
        edgeY = cameraOrigin.y;
    } else if (util::math::equal(angle, (float)M_PI / 2.f) || util::math::equal(angle, (float)-M_PI / 2.f)) {
        // vertical line
        edgeX = cameraOrigin.x;
        edgeY = cameraOrigin.y + halfh;
    } else {
        float yLeft = cameraOrigin.y + halfw * slope;
        if (yLeft >= cameraOrigin.y - halfh && yLeft <= cameraOrigin.y + halfh) {
            edgeX = cameraOrigin.x - halfw;
            edgeY = yLeft;
        } else {
            float yRight = cameraOrigin.y - halfw * slope;
            edgeX = cameraOrigin.x + halfw;
            edgeY = yRight;
        }
    }

    this->setPosition(edgeX, edgeY);
}

PlayerProgressArrow* PlayerProgressArrow::create() {
    auto ret = new PlayerProgressArrow;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}
