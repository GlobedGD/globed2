#pragma once

#include "_internal.hpp"

namespace globed::settings {
// Returns the path to the currently used save file for Globed settings.
Result<std::filesystem::path> getActiveSlotPath();

// Returns the internal settings container. If you want to make modifications, you should call `reloadSaveContainer`
// with the modified value.
Result<matjson::Value> getActiveSlotContainer();

// Reload the settings container from the given container and save it to the disk.
Result<void> reloadSaveContainer(const matjson::Value &container);

// Get whether a specific launch flag was set this session.
// The launch flag can be both prefixed and unprefixed, e.g. `globed-tracing` or `tracing` will be considered the same.
Result<bool> getLaunchFlag(std::string_view key);
} // namespace globed::settings

// Implementation

namespace globed::settings {
inline Result<std::filesystem::path> getActiveSlotPath()
{
    return _internal::request<std::filesystem::path>(_internal::Type::ActiveSlotPath);
}

inline Result<matjson::Value> getActiveSlotContainer()
{
    return _internal::request<matjson::Value>(_internal::Type::ActiveSlotContainer);
}

inline Result<void> reloadSaveContainer(const matjson::Value &container)
{
    return _internal::requestVoid<matjson::Value>(_internal::Type::ReloadSaveContainer, container);
}

inline Result<bool> getLaunchFlag(std::string_view key)
{
    return _internal::request<bool, std::string_view>(_internal::Type::LaunchFlag, key);
}
} // namespace globed::settings
