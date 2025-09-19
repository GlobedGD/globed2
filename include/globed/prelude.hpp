#pragma once

#include "config.hpp"
#include "util/singleton.hpp"
#include "util/CStr.hpp"
#include "util/color.hpp"
#include "util/ConstexprString.hpp"
#include "util/assert.hpp"
#include "util/lazy.hpp"
#include "util/Random.hpp"
#include "core/SettingsManager.hpp"
#include "core/PopupManager.hpp"
#include "core/Core.hpp"
#include "core/Constants.hpp"
#include "core/actions.hpp"
#include <Geode/Result.hpp>

namespace globed {

template <typename T = void, typename E = std::string>
using Result = geode::Result<T, E>;

using geode::Ok;
using geode::Err;

namespace log = geode::log;

}

template <typename T>
T* get() {
    return globed::cachedSingleton<T>();
}
