// Global setup stuff
#include <Geode/Geode.hpp>
#include <globed/core/SettingsManager.hpp>
#include <qunet/Log.hpp>

using namespace geode::prelude;

$execute {
    qn::log::setLogFunction([](qn::log::Level level, const std::string& message) {
        switch (level) {
            case qn::log::Level::Debug: {
                if (globed::setting<bool>("core.dev.net-debug-logs")) {
                    log::debug("[Qunet] {}", message);
                }
            } break;

            case qn::log::Level::Info: {
                log::info("[Qunet] {}", message);
            } break;

            case qn::log::Level::Warning: {
                log::warn("[Qunet] {}", message);
            } break;

            case qn::log::Level::Error: {
                log::error("[Qunet] {}", message);
            } break;
        }
    });
}
