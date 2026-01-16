# Globed API

If you want to make a mod that extends Globed's functionality or otherwise interacts with some parts of the mod, the link API is the simplest way to do so, and it lets you access majority of Globed's classes (nearly everything besides UI). This API requires adding the mod as a required dependency and linking to the mod. If you do not wish to do so, the [Soft Link API](./soft-link-api.md) can be used instead, however it has less features.

## Setup

First add Globed as a dependency in your mod's `mod.json`:

```json
{
  "dependencies": {
    "dankmeme.globed2": ">=2.0.0"
  }
}
```

Then add the following lines in your mod's `CMakeLists.txt` (if you don't know where, just put them at the bottom or below `setup_geode_mod`):

```cmake
CPMAddPackage("gh:zhihaoy/nontype_functional#bdb0987")
CPMAddPackage("gh:dankmeme01/asp2#05234957")
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/geode-deps/dankmeme.globed2/include)
target_link_libraries(${PROJECT_NAME} PRIVATE std23::nontype_functional asp)
```

(this is required because Geode has no way to know that Globed uses these libraries in its headers)

## Usage

If interacting with in-game parts, or really anything that might benefit from what's described below, you may want to make a module:

```cpp
#include <globed/core/ModuleCrtp.hpp>
#include <globed/core/Core.hpp>

// ModuleCrtpBase creates a `get` function that does the following, if the module hasn't been created yet:
// 1. Create the module and add it to the Core
// 2. Call the `onModuleInit` function, if it exists
// It is purely a convenience class, if you want higher control you may choose to simply inherit `globed::Module`.
class MyModule : public globed::ModuleCrtpBase<MyModule> {
public:
    void onModuleInit() {
        // Enable the hooks of this module only when in a level and connected to a server
        this->setAutoEnableMode(globed::AutoEnableMode::Level);
    }

    virtual std::string_view name() const override {
        return "My Module";
    }
    virtual std::string_view id() const override {
        return "dev.my-module";
    }
    virtual std::string_view author() const override {
        return "Dev";
    }
    virtual std::string_view description() const override {
        return "This module adds cool stuff!";
    }

private:
    // Module adds a lot of virtual functions you can override to hook into various events.
    // Check Module.hpp to see everything available

    Result<> onEnabled() override {
        // perform init logic ..
        return Ok();
    }

    Result<> onDisabled() override {
        // perform cleanup logic ..
        return Ok();
    }

    void onPlayerJoin(globed::GlobedGJBGL* gjbgl, int accountId) override {
        log::info("Player {} has joined the level!", accountId);
    }

    void onPlayerDeath(globed::GlobedGJBGL*, globed::RemotePlayer* player, const globed::PlayerDeath&) override {
        log::info("Player {} has died!", player->displayData().username);
    }
};

// If you want hooks to only be enabled when the module is, you can call `claimHook`/`claimPatch` or use the claim macro for $modify:
class $modify(PlayLayer) {
    static void onModify(auto& self) {
        GLOBED_CLAIM_HOOKS(MyModule::get(), self,
            "PlayLayer::destroyPlayer",
            "PlayLayer::onQuit",
            // etc..
        );
    }

    void destroyPlayer(PlayerObject* po, GameObject* go) {}
    void onQuit() {}
};

$execute {
    // For good measure, always call ::get() at least once in $execute or $on_mod, to ensure your module really does get created.
    auto& module = MyModule::get();
}
```

Unfortunately, as Globed is a large mod, most of the other stuff is undocumented. You can try checking out various headers and the source code to figure out what you can do. Below is a bunch of examples of what else is possible with this API.

### Receiving and sending network messages

```cpp
#include <globed/core/net/NetworkManager.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/util/format.hpp>

using namespace globed::msg;
auto& nm = globed::NetworkManager::get();

// Check if the user is connected to the server
bool isConnected = nm.isConnected();

// Use a RAII message listener
globed::MessageListener listener = nm.listen<LevelDataMessage>([](const auto& msg) {
    for (const globed::InEvent& event : msg.events) {
        // handle potentially custom events
        if (event.is<globed::UnknownEvent>()) {
            auto& ev = event.as<globed::UnknownEvent>();
            if (ev.type == 0x1234) {
                log::info("Received custom event: {}", globed::hexEncode(ev.rawData));
            }
        }
    }

    return globed::ListenerResult::Continue;
});

// You can set its priority (0 by default) and thread safety (false by default)
listener->setPriority(-10);    // called before other listeners with priority 0
listener->setThreadSafe(true); // may be called on other threads than main

// Store it somewhere, otherwise it will be destroyed at the end of the scope
// m_fields->m_listener = std::move(listener);

// For a listener that is global, use `listenGlobal`:
auto* listener2 = nm.listenGlobal<CentralLoginOkMessage>([](const auto& msg) {
    log::info("Logged in!");
    return globed::ListenerResult::Continue;
});

// `removeListener` removes and deallocates the listener.
nm.removeListener(typeid(CentralLoginOkMessage), listener2);

// Send a game event with custom data
nm.queueGameEvent(globed::UnknownEvent {
    .type = 0x1234,
    .rawData = globed::hexDecode("deadbeef").unwrap(),
});
```

### Accessing settings and save values

```cpp
// For a full list of settings, check src/core/SettingsManager.cpp
auto& sm = globed::SettingsManager::get();

// Get/set a builtin setting
auto ac = sm.setting<bool>("core.autoconnect"); // or globed::setting<...>(...)
log::debug("Autoconnect: {}", (bool)ac);
ac = !ac;

// Create a custom setting
sm.registerSetting("my-extension.my-cool-setting", 2.5f);
sm.registerLimits("my-extension.my-cool-setting", 0.0f, 5.0f); // optional - set limits
sm.registerValidator("my-extension.my-cool-setting", [](const auto& v) { return v.isNumber(); }); // optional - set validator

// Reset all settings
sm.reset();

// Modify, and switch between save slots
std::vector<globed::SaveSlotMeta> slots = sm.getSaveSlots();
sm.renameSaveSlot(0, "hi first slot");
sm.deleteSaveSlot(1);
sm.switchToSaveSlot(42);

// Write any changes to disk
sm.commitSlotsToDisk();

// Get/set custom values (these don't have to be registered, are arbitrary)
std::optional<bool> idk = globed::value<bool>("core.custom.idk");
globed::setValue<bool>("core.custom.idk", true);
```

### Playing around with audio

```cpp
auto& am = globed::AudioManager::get();

am.setRecordBufferCapacity(3); // Optional

// Start recording audio
am.startRecordingEncoded([](const globed::EncodedAudioFrame& frame) {
    // `frame` now contains up to 3 opus frames, each is just a byte array
    for (auto& oframe : frame.getFrames()) {
        log::debug("Opus frame of size: {} @ {}", oframe.size, fmt::ptr(oframe.data.get()));
    }
}).unwrap();

// Or, start recording raw PCM audio
am.startRecordingRaw([](const float* pcm, size_t samples) {
    log::debug("Got {} PCM samples", samples);
    // play the audio back through the stream with ID -1
    globed::AudioManager::get().playFrameStreamedRaw(-1, pcm, samples);
}).unwrap();

// When in a level, each player that speaks has their own stream with ID being their account ID:
int playerId = 123456;
bool speaking = am.isStreamActive(playerId);
float volume = am.getStreamVolume(playerId);
float loudness = am.getStreamLoudness(playerId);
am.stopOutputStream(playerId);

// Enable/disable deafen
am.setDeafen(true);
```

### Getting data from various singletons

```cpp
// Retrieve data about a player (icons, username, etc)
auto& pcm = globed::PlayerCacheManager::get();
std::optional<globed::PlayerDisplayData> data = pcm.get(1234);
if (data) {
    log::info("Player username: {}, cube: {}, color 1: {}", data->username, data->icons.cube, data->icons.color1.inner());
}

// Check the current room information
auto& rm = globed::RoomManager::get();
bool inRoom = rm.isInRoom();
bool isOwner = rm.isOwner();
uint32_t roomId = rm.getRoomId();
globed::RoomSettings& settings = rm.getSettings();

// Check if a certain user is a friend or blocked
auto& flm = globed::FriendListManager::get();
if (flm.isLoaded()) {
    bool isFriend = flm.isFriend(1234);
    bool isBlocked = flm.isBlocked(4321);
}

// Simulate keypresses
auto& km = globed::KeybindsManager::get();
km.handleKeyDown(enumKeyCodes::KEY_A);
km.handleKeyUp(enumKeyCodes::KEY_A);
```
