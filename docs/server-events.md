# Server Events

Server events are a feature of Globed that allows the client to send arbitrary data to the server, which may then be forwarded to other players. This guide will explain in-depth how to send events and listen to them. Alternatively, for a quick start you can check [this file](https://github.com/GlobedGD/globed-api-example/blob/main/src/Events.cpp) in the example mod.

## Limitations (please read)

This functionality is offered on the **Fair Use** basis. If we detect unfaithful use of events, we reserve the right to add stricter limits, or even completely disable this feature server-side if heavy abuse is detected. If you want to use server events in your mod, please adhere to the following recommendations:

* Don't cause spam - don't send more events than necessary. If you need to sync a value that doesn't change often, send events only when a change is made. While events are highly flexible, we don't want you using them to make your own real-time shooter game that sends 60 updates per second; bandwidth is not free.
* Scope events whenever possible - by default they are sent to everyone in the same session/rooms. If the event should only be sent to a few specific people, then don't send it to everybody.
* Limit the data sent - again, bandwidth is not free. If you have an event that is sent fairly often and contains plenty of data, we recommend using more efficient encoding. For example, prefer using binary encoding over JSON or other formats. We recommend using the [dbuf](https://github.com/dankmeme01/dbuf) library for easy binary encoding/decoding.

## Defining an event

Below is an example of a basic skeleton for a server event
```cpp
// not included by soft-link header!
#include <dankmeme.globed2/include/globed/core/Event.hpp>

// if you included dbuf, you can use it for easy binary serialization
#include <dbuf/ByteWriter.hpp>
#include <dbuf/ByteReader.hpp>

// Second argument specifies the server to which this event can be sent, more information below
struct MyEvent : globed::ServerEvent<MyEvent, globed::EventServer::Game> {
    // String ID that uniquely identifies the event, must include the mod ID there
    static constexpr auto Id = "my-event"_spr;

    // Custom fields, this can be whatever you want
    int playerId = 0;

    MyEvent(int id) : playerId(id) {}

    // This function encodes the event struct into bytes that will be sent to other users
    // Encoding is unspecified, i.e. this can return a utf-8 string or binary data, or nothing
    // This function may be omitted, but then `send()` will be inaccessible
    std::vector<uint8_t> encode() const {
        dbuf::ByteWriter wr;
        wr.writeI32(this->playerId);
        return std::move(wr).intoInner();
    }

    // This function decodes the binary event data into this event struct.
    // Return an error if the data is malformed, the event will be ignored and the error will be printed.
    // This function may be omitted, but then `listen()` will be inaccessible
    static geode::Result<MyEvent> decode(std::span<const uint8_t> data) {
        dbuf::ByteReader reader{data};
        MyEvent out{};

        GEODE_UNWRAP_INTO(out.id, reader.readI32());

        return Ok(std::move(out));
    }
};
```

Every event should specify whether it will be used only on the game server, the central server, or both. Sending and receiving will fail if this is incorrectly specified. Choose the server depending on event's purpose:
* Central server: can be sent to anybody online without being in a level, flexible but may have higher latency
* Game server: can only be sent to people in the same session as you, more efficient and fast

Server is specified in the template argument: e.g. `ServerEvent<T, EventServer::Both>`. It can be omitted, then it will default to `Both` so the event will be usable with both servers.

All events *should* have a globally unique ID in the form `mod.id/event-id`. There are no constraints on the event ID after the slash, besides that it should be shorter than 127 characters. `"mod.id/event-id"`, `"event-id"_spr`, `"my_epic.event"_spr` are all valid IDs.

## Sending / receiving

Sending an event is straightforward:

```cpp
MyEvent event{42};

// Sends this event to the *default* server.
// If the event is enabled specifically only on central/game server, it sends it to the enabled server.
// If the event is enabled for both servers, sends it to the *central* server.
event.send();

// Sends this event to the game server
event.send(globed::EventServer::Game);
// Sends this event to both servers, if applicable
event.send(globed::EventServer::Both);
```

You can specify extra options via the `EventOptions` struct:

```cpp
globed::EventOptions opts{};
opts.server = globed::EventServer::Both;
opts.reliable = true; // if 'true' (default), event must be re-delievered if lost due to packet loss
opts.urgent = true;   // if 'true', event is sent with minimal delay; otherwise it may be queued for a bit

// if this vector is non-empty, event is only sent to players whose account IDs are in it
opts.targetPlayers.push_back(1234);

event.send(opts);
```

Listening is nearly the same as listening to a Geode event, with the exception that you cannot swallow events:

```cpp
auto handle = MyEvent::listen([](const MyEvent& event) {
    log::info("Received event with {}", event.playerId);
});

// alternate overload that allows you to check event options (e.g. ID of the player who sent it)
auto handle = MyEvent::listen([](const MyEvent& event, const globed::EventOptions& opts) {
    log::info("Received event with {} from player {}", event.playerId, opts.sender);
});
```

In case your listener needs to unregister itself (for example, you close a popup in the lambda which should destroy the listener), care must be taken to return `globed::SkipRemainingEvents{}`. Otherwise, if the event is queued more than once in a single batch, your callback will **always** be called multiple times, which may lead to UB:

```cpp
m_handle = MyEvent::listen([](const MyEvent& event) {
    this->onClose(nullptr);

    // this ensures the lambda will NOT be called again while processing this batch of events,
    // and avoids a use-after-free if the listener got deallocated here
    return globed::SkipRemainingEvents{};
});
```

## Low-level interface

If the high-level, CRTP based interfaces above are not enough and you want more control, you may choose to use the lower-level APIs.

Register your events once on game launch:
```cpp
globed::api::waitForGlobed([] {
    globed::api::net::registerEvent("my-event"_spr, globed::EventServer::Both);
});
```

To send:
```cpp
std::vector<uint8_t> data{ /* ... */ };
globed::api::net::sendEvent("my-event"_spr, std::move(data), EventOptions{});
```

To receive, listen to the `EventsMessage` manually and filter the needed events:
```cpp
globed::MessageEvent<globed::msg::EventsMessage>{false}.listen([](const auto& data) {
    for (auto& ev : data.events) {
        if (ev.name != "my-event"_spr) continue;

        log::info("Received my event: {}", ev.data);
    }
});
```
