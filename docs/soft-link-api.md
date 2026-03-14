# Globed Soft Link API

Soft link API is a way to interact with globed in a way that does not require linking to the mod (this means, Globed does not have to be a dependency of your mod at all). The way it works is your mod requests a function table using Geode events, and then caches it and uses that table to call various Globed functions. This is less efficient, less powerful and more tedious than the [Link API](./link-api.md) but a huge upside is that you aren't forcing users of your mod to have Globed enabled.

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

Result<bool> res = globed::api()->net.isConnected();
```

You can take a look at other available functions in your intellisense. Current available subtables are: `net`, `game`, `player`.

This API is currently not very complete, and a fairly small subset of functions are available. If you want to use something not exposed here, use the Link API or make a PR or an issue telling us what should be added.
