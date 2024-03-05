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

    auto gm = GameManager::get();
    auto color1 = gm->colorForIdx(data.color1);
    auto color2 = gm->colorForIdx(data.color2);

    Build<SimplePlayer>::create(data.cube)
        .color(color1)
        .secondColor(color2)
        .opacity(static_cast<uint8_t>(settings.levelUi.progressOpacity * 255))
        .scale(0.5f)
        .parent(this)
        .store(playerIcon);

    if (data.glowColor != -1) {
        playerIcon->setGlowOutline(gm->colorForIdx(data.glowColor));
    } else {
        playerIcon->disableGlowOutline();
    }
}

void PlayerProgressArrow::updatePosition(
        CCPoint cameraOrigin,
        CCSize cameraCoverage,
        CCPoint visibleOrigin,
        CCSize visibleCoverage,
        CCPoint playerPosition,
        float zoom
) {
    auto cameraCenter = cameraOrigin + cameraCoverage / 2.f;
    float cameraLeft = cameraOrigin.x;
    float cameraRight = cameraOrigin.x + cameraCoverage.width;
    float cameraBottom = cameraOrigin.y;
    float cameraTop = cameraOrigin.y + cameraCoverage.height;

    auto visibleCenter = visibleOrigin + visibleCoverage / 2.f;
    float visibleLeft = visibleOrigin.x;
    float visibleRight = visibleOrigin.x + visibleCoverage.width;
    float visibleBottom = visibleOrigin.y;
    float visibleTop = visibleOrigin.y + visibleCoverage.height;

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

    distance *= zoom;

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
