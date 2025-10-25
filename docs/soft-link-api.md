# Globed Soft Link API

Soft link API is a way to interact with globed in a way that does not require linking to the mod (this means, Globed does not have to be a dependency of your mod at all). The way it works is your mod requests a function table using Geode events, and then caches it and uses that table to call various Globed functions. This is less efficient, less powerful and more tedious than the [Link API](./link-api.md) but a huge upside is that you aren't forcing users of your mod to have Globed enabled.

## Setup

Add Globed as a dependency in your mod's `mod.json`:

```json
{
    "dependencies": {
        "dankmeme.globed2": {
            "importance": "suggested",
            "version": ">=2.0.0"
        }
    }
}
```

Completely optional, but the following line can be added to your `CMakeLists.txt` to make including headers easier:

```cmake
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/geode-deps/dankmeme.globed2/include)
```

Additionally, to invoke any API methods that take in a function object, `nontype_functional` must be included in your project's cmake:

```cmake
CPMAddPackage("gh:zhihaoy/nontype_functional#bdb0987")
target_link_libraries(${PROJECT_NAME} std23::nontype_functional)
```

## Usage

TODO, this is a lot of work oh god

```cpp
// One of the two, depending on whether you added the line of cmake above
#include <dankmeme.globed2/include/globed/soft-link/API.hpp>
#include <globed/soft-link/API.hpp>

Result<bool> res = globed::api()->isConnected();
```
