#pragma once

#include "aliases.hpp"
#include "config.hpp"
#include "util/singleton.hpp"
#include "util/color.hpp"
#include "util/ConstexprString.hpp"
#include "util/assert.hpp"
#include "util/lazy.hpp"
#include "core/SettingsManager.hpp"
#include "core/PopupManager.hpp"
#include "core/Core.hpp"
#include "core/Constants.hpp"
#include "core/actions.hpp"

namespace globed {

template <typename T>
T* get() {
    return globed::singleton<T>();
}

}

#define GLOBED_NOCOPY(cls) \
    cls(const cls&) = delete; \
    cls& operator=(const cls&) = delete

#define GLOBED_NOMOVE(cls) \
    cls(cls&&) = delete; \
    cls& operator=(cls&&) = delete
