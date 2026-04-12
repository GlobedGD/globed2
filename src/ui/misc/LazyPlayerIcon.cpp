#include "LazyPlayerIcon.hpp"
#include <core/preload/PreloadManager.hpp>
#include <globed/core/data/PlayerDisplayData.hpp>
#include <globed/util/gd.hpp>

using namespace geode::prelude;

namespace globed {

bool LazyPlayerIcon::init(const Icons& icons) {
    // if all icons have already been preloaded, we can skip the lazy loading
    auto& pm = PreloadManager::get();
    if (pm.iconsLoaded()) {
        return PlayerIcon::init(icons);
    }

    // otherwise, let's initialize the default icons and then load the real ones asynchronously
    if (!PlayerIcon::init(convertPlayerIcons(DEFAULT_PLAYER_DATA.icons))) return false;

    this->updateIcons(icons);

    return true;
}

void LazyPlayerIcon::updateIcons(const Icons& icons) {
    auto& pm = PreloadManager::get();
    if (pm.iconsLoaded()) {
        return PlayerIcon::updateIcons(icons);
    }

    if (m_waiting) {
        log::warn("LazyPlayerIcon::updateIcons called when already waiting, ignoring");
        return;
    }

    m_realIcons = icons;
    m_waiting = true;

    PreloadOptions opts{};
    opts.blocking = false;
    opts.completionCallback = [wref = WeakRef{this}] {
        auto self = wref.lock();
        if (!self) return;

        self->onLoaded();
    };
    pm.loadIcon(icons.type, icons.id, std::move(opts));
}

void LazyPlayerIcon::onLoaded() {
    PlayerIcon::updateIcons(m_realIcons);
    m_waiting = false;
}

LazyPlayerIcon* LazyPlayerIcon::create(const Icons& icons) {
    auto ret = new LazyPlayerIcon;
    if (ret->init(icons)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

}
