#pragma once

#include <Geode/loader/Loader.hpp>

namespace globed {
    // Returns whether Globed is currently enabled and loaded or not.
    inline bool isLoaded() {
        return geode::Loader::get()->getLoadedMod("dankmeme.globed2") != nullptr;
    }
}
