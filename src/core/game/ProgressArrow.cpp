#include <core/hooks/GJBaseGameLayer.hpp>
#include <globed/core/SettingsManager.hpp>
#include <globed/core/game/ProgressArrow.hpp>

#include <UIBuilder.hpp>
#include <cue/Util.hpp>

using namespace geode::prelude;

namespace globed {

bool ProgressArrow::init()
{
    if (!CCNode::init())
        return false;

    this->updateIcons(cue::Icons{});

    return true;
}

void ProgressArrow::updateIcons(const cue::Icons &icons)
{
    cue::resetNode(m_icon);

    float progressOpacity = globed::setting<float>("core.level.progress-opacity");

    m_icon = Build(cue::PlayerIcon::create(icons)).scale(0.5f).anchorPoint(0.5f, 0.5f).parent(this);
}

void ProgressArrow::updatePosition(const GameCameraState &camState, CCPoint playerPosition, double angle)
{
    // angle is between 0 and 1, stretch it back to [0; 2pi)
    angle *= M_PI * 2;

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
    bool inCameraCoverage = (playerPosition.x >= cameraLeft && playerPosition.x <= cameraRight &&
                             playerPosition.y >= cameraBottom && playerPosition.y <= cameraTop);

    if (inCameraCoverage) {
        this->setVisible(false);
        return;
    }

    this->setVisible(true);

    float distance =
        std::sqrt(std::pow(playerPosition.x - cameraCenter.x, 2) + std::pow(playerPosition.y - cameraCenter.y, 2));

    distance *= camState.zoom;

    float indicatorX = visibleCenter.x + distance * std::cos(angle);
    float indicatorY = visibleCenter.y + distance * std::sin(angle);

    indicatorX = std::max(visibleLeft + 10.f, std::min(visibleRight - 10.f, indicatorX));
    indicatorY = std::max(visibleBottom + 10.f, std::min(visibleTop - 10.f, indicatorY));

    this->setPosition(indicatorX, indicatorY);
}

ProgressArrow *ProgressArrow::create()
{
    auto ret = new ProgressArrow;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

} // namespace globed