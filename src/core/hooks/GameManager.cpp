#include <globed/config.hpp>
#include <core/PreloadManager.hpp>

#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include <asp/time.hpp>

using namespace geode::prelude;

namespace globed {

struct GLOBED_MODIFY_ATTR HookedGameManager : geode::Modify<HookedGameManager, GameManager> {
    CCTexture2D* loadIcon(int id, int type, int requestId) {
        // auto start = asp::time::Instant::now();

        auto& pm = PreloadManager::get();

        if (auto tex = pm.getCachedIcon(type, id)) {
            // log::debug("loadIcon fast exit: {}", start.elapsed().toString());
            return tex;
        }

        auto texture = GameManager::loadIcon(id, type, -1);
        if (texture) {
            pm.setCachedIcon((int) type, id, texture);
        } else {
            log::warn("loadIcon returned a null texture, icon ID = {}, icon type = {}", id, type);
        }

        // log::debug("loadIcon slow exit: {}", start.elapsed().toString());
        return texture;
    }

    $override
    void unloadIcon(int iconId, int iconType, int idk) {
        // intentionally do nothing
    }
};

}

