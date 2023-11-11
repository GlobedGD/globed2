# Contributor guide

This is a small guide mostly about helper macros and utilities you can use in the code. It is highly recommended to use everything listed here instead of the alternatives, to keep the code clean and consistent.

## Utilities

Use util classes or methods whenever possible. For example, instead of using `rand()` or manually creating a random engine, use `util::rng::Random`:

```cpp
auto num = Random::get().generate<uint32_t>(0, 10000);
```

Short descriptions of available utility namespaces (`util::` omitted for simplicity sake):

* `collections` - provides various collections like `CappedQueue` and util methods for working with other collections.
* `crypto` - provides cryptography utilities like secure hashing, TOTP generation, base64, hex encoding/decoding.
* `data` - provides helpers `byte`, `bytearray` and `bytevector` and byteswap implementations.
* `debugging` - provides a `Benchmarker` class and other utilities for debugging code.
* `net` - provides networking utilities.
* `rng` - provides a simple interface for generating random numbers (and floats!)
* `sync` - provides various classes for exchanging the data between threads safely.
* `time` - provides utilities for getting the current system time and formatting it to a string.

## Unexpected conditions

For general unexpected conditions, use `GLOBED_ASSERT`, like so:

```cpp
GLOBED_ASSERT(1 != 2, "math is broken");
// if 1 == 2, throws std::runtime_error with the message "Assertion failed: math is broken"
```

For something that should **never, ever happen** (not even in case of network data corruption), use `GLOBED_HARD_ASSERT` or `util::debugging::suicide`:

```cpp
GLOBED_HARD_ASSERT(sizeof(void*) != 2, "why are we in 16-bit mode again??")
// if sizeof(void*) == 2, prints "Assertion failed: why are we in 16-bit mode again??" and terminates the entire game.

util::debugging::suicide();
// similar to GLOBED_HARD_ASSERT(false) but quicker and with less info printing
```

If you want to terminate the game as quickly as possible (why?), use `GLOBED_SUICIDE`:

```cpp
[[noreturn]] void crash() {
    // raises SIGTRAP / EXCEPTION_BREAKPOINT by default
    GLOBED_SUICIDE;
}
```

For code that is not implemented yet (or for stubs that should never be implemented but must be defined), use `GLOBED_UNIMPL`:

```cpp
void dontCallMe() {
    GLOBED_UNIMPL("dontCallMe")
}
// if called, throws std::runtime_error with the message "Assertion failed: unimplemented: dontCallMe"
```

Packet subclasses have their own variations of this, `GLOBED_PACKET_DECODE_UNIMPL` and `GLOBED_PACKET_ENCODE_UNIMPL`:

```cpp
class MyPacket : public Packet {
    // ...
    GLOBED_PACKET_ENCODE { /* ... */ }
    GLOBED_PACKET_DECODE_UNIMPL
    // if this is a client-side packet, there is no point of making a decode method.
    // attempting to decode this packet throws std::runtime_error with the message
    // "Assertion failed: unimplemented: Decoding unimplemented for packet <packet id>"
};
```

## Singletons

For singleton classes, put the `GLOBED_SINGLETON(cls)` at the top of the class declaration:

```cpp
class MySingleton {
    GLOBED_SINGLETON(MySingleton)
    // your other methods
};
```

You also must put `GLOBED_SINGLETON_DEF(cls)` in the class definition (usually your `.cpp` file), so that it can find certain internal members:

```cpp
GLOBED_SINGLETON_DEF(MySingleton)
```

If you did everything correctly, you should now have a singleton class with a thread-safe `get()` method that will lazily create a new instance when called for the first time, and in future will return that existing instance. Note that the underlying singleton is not thread-safe by default, only the `get()` method that constructs/retrieves it.
