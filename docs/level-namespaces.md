# Level Namespaces

Some mods have the user join a level that is *not* a real level on the GD servers. For example, you might have a fangame mod that uses level ID 3 for one of the levels. When joining that level with Globed enabled, it will treat that level as the level ID 3 in the official servers, aka Polargeist, which is likely not what you want.

You may be able to just use a random high number for your IDs, but that's obviously not scalable and often undesirable. For this, Globed v2.3.0 adds **Level Namespaces**: when you join a level you don't only use the level ID but also a unique **namespace ID** to distingish non-GD levels. When you join a level with ID 3 and namespace 1, you are no longer playing with people that are on Polargeist, since the namespace for GD levels is 0.

## How to use

Globed uses a `ResolveLevelIdEvent` event to find out which namespace should be used for a level. This event is posted inside a `GJBaseGameLayer::init` hook, before calling the original, with the level being joined. If you want to override the namespace for that level, your mod should write the unique namespace ID and the level ID you want to use:

```cpp
#include <dankmeme.globed2/soft-link/Events.hpp>

static constexpr uint32_t MY_NAMESPACE_ID = 123456;

$on_mod(Loaded) {
    ResolveLevelIdEvent().listen([](GJGameLevel* level, LevelId* outId) {
        // If this is called, the user is joining a level
        // Determine if this is our custom level that we want to override the ID for

        // How to do this is up to you, for example you could store a global flag or tag the GJGameLevel
        // For sake of a simple example, we will assume that any level with ID 3 is our custom level
        if (level->m_levelID == 3) {
            // Override the values
            outId->ns = MY_NAMESPACE_ID;
            outId->id = 3;

            // It is very important to return true if you are overriding the level
            return true;
        }

        return false;
    }).leak();
}
```

## What ID to pick

We highly suggest that you generate a random 32-bit integer and save it as a constant in your code, to minimize risk of collision. Below is a list of known namespace IDs that you should not use, and you can PR additions if you use any in your own mod:

* 0 - vanilla GD levels
* 1 - Editor Collab
