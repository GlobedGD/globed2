# Globed API

Globed provides event-based API headers that can be used without linking, which means you can communicate with the mod without making it a dependency.

## Setup

To use the API, add Globed to the dependency list in your mod's `mod.json` like so:

```json
"dependencies": {
    "dankmeme.globed2": {
        "importance": "suggested",
        "version": ">=1.8.0"
    }
}
```

After that, you can include headers in any file by typing

```cpp
#include <dankmeme.globed2/include/net.hpp>
#include <dankmeme.globed2/include/settings.hpp>
// etc..

// or, to include every single header at once
#include <dankmeme.globed2/include/globed.hpp>
```

## General

(Include: `dankmeme.globed2/include/general.hpp`)

### isLoaded

Returns whether Globed is currently enabled and loaded or not.

```cpp
bool globed::isLoaded();
```

## Player

(Include: `dankmeme.globed2/include/player.hpp`)

### isGlobedPlayer

Returns whether the given `PlayerObject` belongs to an online player.

```cpp
Result<bool> globed::player::isGlobedPlayer(PlayerObject* node);
```

### isGlobedPlayerFast

Returns whether the given `PlayerObject` belongs to an online player. Is significantly faster than `isGlobedPlayer`, but may report false positives or false negatives, as it relies on the tag value on the player object. Only use this function if `isGlobedPlayer` is an actual performance concern.

```cpp
bool globed::player::isGlobedPlayerFast(PlayerObject* node);
```

### accountIdForPlayer

Returns the account ID of a player by their `PlayerObject`. If the given object does not represent an online player, -1 is returned.

```cpp
Result<int> globed::player::accountIdForPlayer(PlayerObject* node);
```

### explodeRandomPlayer

Explodes a random player in the level. I will not elaborate further.

```cpp
Result<void> globed::player::explodeRandomPlayer();
```

### playersOnLevel

Returns the amount of players on the current level. If the player is not in a level, returns an error.

```cpp
Result<size_t> globed::player::playersOnLevel();
```

### playersOnline

Returns the amount of players online. Might not be completely accurate. If the player is not connected to a server, returns an error.

```cpp
Result<size_t> globed::player::playersOnline();
```

## Networking

(Include: `dankmeme.globed2/include/net.hpp`)

### isConnected

Returns whether the player is currently connected to a server.

```cpp
Result<bool> globed::net::isConnected();
```

### getServerTps

Returns the TPS (ticks per second) of the server, or 0 if not connected. This value indicates how many times player data is sent between client and server every second, while in a level.

```cpp
Result<uint32_t> globed::net::getServerTps();
```

### getPing

Returns the current latency to the server, or -1 if not connected.

```cpp
Result<uint32_t> globed::net::getPing();
```

### isStandalone

Returns whether the user is connected to a standalone server.

```cpp
Result<bool> globed::net::isStandalone();
```

### isReconnecting

Returns whether a connection break happened and the client is currently trying to reconnect.

```cpp
Result<bool> globed::net::isReconnecting();
```

## Admin

(Include: `dankmeme.globed2/include/admin.hpp`)

### isModerator

Returns whether the user is connected to a server, and has moderation powers on the server.

```cpp
Result<bool> globed::admin::isModerator();
```

### isAuthorizedModerator

Returns whether the user is connected to a server, has moderation powers on the server, and is logged into the admin panel.

```cpp
Result<bool> globed::admin::isAuthorizedModerator();
```

### openModPanel

Opens the moderation panel. Does nothing if the user is not a moderator.

```cpp
Result<void> globed::admin::openModPanel();
```

## Settings

(Include: `dankmeme.globed2/include/settings.hpp`)

### getActiveSlotPath

Returns the path to the currently used save file for Globed settings.

```cpp
Result<std::filesystem::path> globed::settings::getActiveSlotPath();
```

### getActiveSlotContainer

Returns the internal settings container. If you want to make modifications, you should call `reloadSaveContainer` with the modified value.

```cpp
Result<matjson::Value> globed::settings::getActiveSlotContainer();
```

### reloadSaveContainer

Reload the settings container from the given container and save it to the disk.

```cpp
Result<void> globed::settings::reloadSaveContainer(const matjson::Value& container);
```

Example usage:

```cpp
auto result = globed::settings::getActiveSlotContainer();
if (!result) {
    // always handle the errors!
    log::warn("Failed to obtain save slot container: {}", result.unwrapErr());
    return;
}

auto& container = result.unwrap();
container["playersnameOpacity"] = 1.0;
auto result2 = globed::settings::reloadSaveContainer(container);
if (!result2) {
    log::warn("Failed to save container: {}", result2.unwrapErr());
}
```

### getLaunchFlag

Get whether a specific launch flag was set this session. The launch flag can be both prefixed and unprefixed, e.g. `globed-tracing` or `tracing` will be considered the same.

For a complete list of launch flags, see their respective [Documentation Page](./launch-args.md)

```cpp
Result<bool> globed::settings::getLaunchFlag(std::string_view key);
```
