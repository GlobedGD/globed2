#include <Geode/utils/Result.hpp>
#include <Geode/loader/Log.hpp>
#include <Geode/loader/Mod.hpp>

using geode::Result;
using geode::Ok;
using geode::Err;
using geode::Mod;
using geode::Patch;
using geode::Loader;

// ugly workaround because MSVC sucks ass
namespace __globed_log_namespace_shut_up_msvc {
    namespace log = geode::log;
}

using namespace __globed_log_namespace_shut_up_msvc;
