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

Packet subclasses have their own variations of this, `GLOBED_DECODE_UNIMPL` and `GLOBED_ENCODE_UNIMPL`:

```cpp
class MyPacket : public Packet {
    // ...
    GLOBED_ENCODE { /* ... */ }
    GLOBED_DECODE_UNIMPL
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

If you did everything correctly, you should now have a singleton class with a thread-safe `get()` method that will lazily create a new instance when called for the first time, and in future will return that existing instance. Note that the underlying singleton is not thread-safe by default, only the `get()` method.

## Platform dependent macros

These are various macros that may differ depending on your compiler, targeted platform, and the `config.hpp` file.

`GLOBED_WIN32`, `GLOBED_MAC`, `GLOBED_ANDROID`, `GLOBED_UNIX`, `GLOBED_X86`, `GLOBED_X86_32`, `GLOBED_X86_64`, `GLOBED_ARM`, `GLOBED_ARM32`, `GLOBED_ARM64` - macros indicating the target platform/architecture. Prefer to use them instead of `GEODE_IS_xxxxx`.

`GLOBED_CAN_USE_SOURCE_LOCATION` - if set to 1, the contents of `<source_location>` are available and `GLOBED_ASSERT`, `GLOBED_HARD_ASSERT` and `GLOBED_UNIMPL` will print the file and line where the assertion failed to the console. It is only set to 1 if either `__cpp_consteval` is defined or `GLOBED_FORCE_CONSTEVAL` is set to 1 in `config.hpp` (unrecommended!).

`GLOBED_SOURCE` - if the previous macro is set to 1, expands to `std::source_location::current()`. Otherwise undefined.

`GLOBED_LITTLE_ENDIAN` - not a macro, but a `constexpr bool` indicating if the target platform is little-endian.

`GLOBED_HAS_FMOD` and `GLOBED_HAS_DRPC` - whether the target platform links to FMOD or has Discord Rich Presence. Defined for each platform in `config.hpp`.

Additionally, if you want to define some values in `config.hpp` manually through build scripts rather than editing `config.hpp`, you should define `GLOBED_IGNORE_CONFIG_HPP` and then define everything else you need.

## Tests

If you want to use any of the code outside of the Geometry Dash/Geode environment (helpful for testing things without launching the game), you must define `GLOBED_ROOT_NO_GEODE` and `GLOBED_TESTING` prior to including any file. This prevents the inclusion of Geode headers and may disable some of the functionality (for example `ByteBuffer::writeColor3` or anything else that relies on Cocos structs).

If you do that, you also must explicitly define `GLOBED_ASSERT_LOG` as the log function or macro for assertion failures. It should take exactly one argument. The recommended definition (and the one used in tests) is:

```cpp
#define GLOBED_ASSERT_LOG(content) (std::cerr << content << std::endl);
```

## Crypto

`util/crypto.hpp` provides some extra macros for cryptography. Note that they don't have the `GLOBED_` prefix.

`CRYPTO_SODIUM_INIT` - initializes sodium_init() if it hasn't been initialized already. It is recommended to call this whenever you are unsure if it has ever been called before.

`CRYPTO_ASSERT(cond, msg)` - same as `GLOBED_ASSERT` but adds `crypto error: ` at the start

`CRYPTO_ERR_CHECK(res, msg)` - same as `CRYPTO_ASSERT(res == 0, msg)`