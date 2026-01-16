#include "GameManager.hpp"
#include <core/hooks/GJBaseGameLayer.hpp>
#include <globed/util/gd.hpp>

using namespace geode::prelude;

constexpr int POP_SCENE_ENUM = 1381341753;
constexpr int NOOP_SCENE_ENUM = 1381341754;

namespace globed {

CCTexture2D *HookedGameManager::loadIcon(int id, int type, int requestId)
{
    // auto start = asp::time::Instant::now();

    auto &pm = PreloadManager::get();

    if (auto tex = pm.getCachedIcon(type, id)) {
        // log::debug("loadIcon fast exit: {}", start.elapsed().toString());
        return tex;
    }

    auto texture = GameManager::loadIcon(id, type, -1);
    if (texture) {
        pm.setCachedIcon((int)type, id, texture);
    } else {
        log::warn("loadIcon returned a null texture, icon ID = {}, icon type = {}", id, type);
    }

    // log::debug("loadIcon slow exit: {}", start.elapsed().toString());
    return texture;
}

void HookedGameManager::unloadIcon(int iconId, int iconType, int idk)
{
    // intentionally do nothing
}

void HookedGameManager::returnToLastScene(GJGameLevel *level)
{
    if (auto lel = LevelEditorLayer::get()) {
        GlobedGJBGL::get(lel)->onQuit();
    }

    if (m_sceneEnum == POP_SCENE_ENUM) {
        globed::popScene();
    } else if (m_sceneEnum == NOOP_SCENE_ENUM) {
        // noop
    } else {
        GameManager::returnToLastScene(level);
    }
}

HookedGameManager &HookedGameManager::get()
{
    return static_cast<HookedGameManager &>(*cachedSingleton<GameManager>());
}

void HookedGameManager::setPopSceneEnum()
{
    m_sceneEnum = POP_SCENE_ENUM;
}

void HookedGameManager::setNoopSceneEnum()
{
    m_sceneEnum = NOOP_SCENE_ENUM;
}

} // namespace globed
