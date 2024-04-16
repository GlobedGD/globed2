#include "progress_arrow.hpp"

#include <managers/settings.hpp>
#include <util/math.hpp>

using namespace geode::prelude;

bool PlayerProgressArrow::init() {
    if (!CCNode::init()) return false;

    this->updateIcons(PlayerAccountData::DEFAULT_DATA.icons);

    return true;
}

void PlayerProgressArrow::updateIcons(const PlayerIconData& data) {
    if (playerIcon) playerIcon->removeFromParent();

    auto& settings = GlobedSettings::get();

    Build<GlobedSimplePlayer>::create(data)
        .opacity(static_cast<uint8_t>(settings.levelUi.progressOpacity * 255))
        .anchorPoint(0.5f, 0.5f)
        .scale(0.5f)
        .parent(this)
        .store(playerIcon);
}

void PlayerProgressArrow::updatePosition(
        const GameCameraState& camState,
        CCPoint playerPosition
) {
    CCSize cameraCoverage = camState.cameraCoverage();

    auto cameraCenter = camState.cameraOrigin + cameraCoverage / 2.f;
    float cameraLeft = camState.cameraOrigin.x;
    float cameraRight = camState.cameraOrigin.x + cameraCoverage.width;
    float cameraBottom = camState.cameraOrigin.y;
    float cameraTop = camState.cameraOrigin.y + cameraCoverage.height;

    auto visibleCenter = camState.visibleOrigin + camState.visibleCoverage / 2.f;
    float visibleLeft = camState.visibleOrigin.x;
    float visibleRight = camState.visibleOrigin.x + camState.visibleCoverage.width;
    float visibleBottom = camState.visibleOrigin.y;
    float visibleTop = camState.visibleOrigin.y + camState.visibleCoverage.height;

    // is the player visible? then dont show the arrow
    bool inCameraCoverage = (
        playerPosition.x >= cameraLeft &&
        playerPosition.x <= cameraRight &&
        playerPosition.y >= cameraBottom &&
        playerPosition.y <= cameraTop
    );

    if (inCameraCoverage) {
        this->setVisible(false);
        return;
    }

    this->setVisible(true);

    float angle = std::atan2(playerPosition.y - cameraCenter.y, playerPosition.x - cameraCenter.x);
    float distance = std::sqrt(std::pow(playerPosition.x - cameraCenter.x, 2) + std::pow(playerPosition.y - cameraCenter.y, 2));

    distance *= camState.zoom;

    float indicatorX = visibleCenter.x + distance * std::cos(angle);
    float indicatorY = visibleCenter.y + distance * std::sin(angle);

    indicatorX = std::max(visibleLeft + 10.f, std::min(visibleRight - 10.f, indicatorX));
    indicatorY = std::max(visibleBottom + 10.f, std::min(visibleTop - 10.f, indicatorY));

    this->setPosition(indicatorX, indicatorY);
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
