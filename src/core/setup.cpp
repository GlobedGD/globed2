// Global setup stuff
#include <Geode/Geode.hpp>
#include <arc/util/Trace.hpp>
#include <asp/Log.hpp>
#include <core/net/NetworkManagerImpl.hpp>
#include <globed/core/SettingsManager.hpp>
#include <qunet/Log.hpp>

using namespace geode::prelude;

$execute
{
    asp::setLogFunction([](asp::LogLevel level, std::string_view msg) {
        switch (level) {
        case asp::LogLevel::Trace: {
#ifdef GLOBED_DEBUG
            log::debug("[asp] {}", msg);
#endif
        } break;

        case asp::LogLevel::Debug: {
            log::debug("[asp] {}", msg);
        } break;

        case asp::LogLevel::Info: {
            log::info("[asp] {}", msg);
        } break;

        case asp::LogLevel::Warn: {
            log::warn("[asp] {}", msg);
        } break;

        case asp::LogLevel::Error: {
            log::error("[asp] {}", msg);
        } break;
        }
    });

    static bool debugEnabled = globed::setting<bool>("core.dev.net-debug-logs");
    globed::SettingsManager::get().listenForChanges<bool>("core.dev.net-debug-logs",
                                                          [](bool value) { debugEnabled = value; });

    qn::log::setLogFunction([](qn::log::Level level, const std::string &message) {
        // globed::NetworkManagerImpl::get().logQunetMessage(level, message);

        switch (level) {
        case qn::log::Level::Debug: {
            if (debugEnabled) {
                log::info("[Qunet] {}", message);
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

    arc::setLogFunction([](std::string message, arc::LogLevel level) {
        // globed::NetworkManagerImpl::get().logArcMessage(level, message);

        switch (level) {
        case arc::LogLevel::Trace: {
#ifdef GLOBED_DEBUG
            log::debug("[arc] {}", message);
#endif
        } break;

        case arc::LogLevel::Warn: {
            log::warn("[arc] {}", message);
        } break;

        case arc::LogLevel::Error: {
            log::error("[arc] {}", message);
        } break;
        }
    });
}
