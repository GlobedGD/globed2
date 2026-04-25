# Globed Soft Link API

Soft link API is a way to interact with globed in a way that does not require linking to the mod (this means, Globed does not have to be a dependency of your mod at all). The way it works is your mod requests a function table using Geode events, and then caches it and uses that table to call various Globed functions. This is more limited than the [Link API](./link-api.md), but it is means you don't need to force users of your mod to install Globed, and it also provides a backwards compatibility guarantee - future versions of Globed may add new functions without breaking old ones.

## Setup

Add Globed as a dependency in your mod's `mod.json`:

```json
{
    "dependencies": {
        "dankmeme.globed2": {
            "version": ">=2.0.0",
            "required": false
        }
    }
}
```

Completely optional, but the following line can be added to your `CMakeLists.txt` to make including headers easier:

```cmake
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/geode-deps/dankmeme.globed2/include)
```

## Usage

```cpp
// One of the two, depending on whether you added the line of cmake above
#include <dankmeme.globed2/include/globed/soft-link/API.hpp>
#include <globed/soft-link/API.hpp>

// Always check if the API is actually available (meaning Globed is loaded and is a compatible version)
// If this returns false, all API functions will return meaningless results (0, "", Err, etc.)
if (globed::api::available()) {
    bool connected = globed::api::net::isConnected();
}

// Note: this will not work, because it requires linking to the mod.
// Only use functions in the `api` namespace, or ones that are inlined in headers.
bool connected = globed::NetworkManager::get().isConnected();
```

You can take a look at other available functions in your intellisense. There are currently four namespaces with functions:
* `api::net` - network related functions
* `api::game` - functions related to the game state (when in a level)
* `api::player` - functions that deal with specific players
* `api::misc` - misc functions that might be useful for mods

This API is currently not very complete, and a fairly small subset of functions are available. If you want to use something not exposed here, use the Link API or make a PR or an issue telling us what should be added.

## Server Events

Server events are a feature of Globed that allows the client to send arbitrary data to the server, which may then be forwarded to other players. This is how Globed implements certain gamemodes (e.g. Switcheroo or 2 player mode) without actually hardcoding any functionality in the server!

TODO


## Backward / forward compatibility

Globed soft-link API maintains both backwards and forwards ABI compatibility - invoking a globed function will not crash no matter if the user has an outdated or a newer version of Globed than you target. If a function does not exist at runtime, it will return a meaningless, default result, so you should be prepared to handle that. Always build against the most recent headers of Globed and check if the functions you use have a doc comment saying which version they were added/removed in. If appropriate, check the version via `globed::api::isAtLeast("vx.x.x")` before calling them.

While there may be exceptions in the future, API functions are generally never removed and thus you don't need to worry about a future version of Globed breaking your mod. If certain functionality was completely removed from Globed, the functions will be kept but will return errors or meaningless values.
