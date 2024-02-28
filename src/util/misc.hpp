#pragma once
#include <defs/essential.hpp>

#include <functional>

#include <data/types/gd.hpp>

// i hate c++
#define _GLOBED_STRNUM "1234567890"
#define _GLOBED_STRLOWER "abcdefghijklmnopqrstuvwxyz"
#define _GLOBED_STRUPPER "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define _GLOBED_STRALPHA _GLOBED_STRLOWER _GLOBED_STRUPPER
#define _GLOBED_STRALPHANUM _GLOBED_STRNUM _GLOBED_STRALPHA
#define _GLOBED_STRSPECIAL "@$#&* {}[],=-().+;'/\"!%^_\\:<>?`~"
#define _GLOBED_STRPRINTABLEINPUT _GLOBED_STRALPHANUM _GLOBED_STRSPECIAL
#define _GLOBED_STRPRINTABLE _GLOBED_STRPRINTABLEINPUT "\r\t\n"
#define _GLOBED_STRURL _GLOBED_STRALPHANUM ":/%._-?#"
#define _GLOBED_STRWHITESPACE " \t\n\r\x0b\x0c"

namespace util::misc {
    constexpr std::string_view STRING_DIGITS = _GLOBED_STRNUM;
    constexpr std::string_view STRING_LOWERCASE = _GLOBED_STRLOWER;
    constexpr std::string_view STRING_UPPERCASE = _GLOBED_STRUPPER;
    constexpr std::string_view STRING_ALPHABET = _GLOBED_STRALPHA;
    constexpr std::string_view STRING_ALPHANUMERIC = _GLOBED_STRALPHANUM;
    constexpr std::string_view STRING_SPECIAL_CHARS = _GLOBED_STRSPECIAL;
    constexpr std::string_view STRING_PRINTABLE = _GLOBED_STRPRINTABLE;
    constexpr std::string_view STRING_PRINTABLE_INPUT = _GLOBED_STRPRINTABLEINPUT;
    constexpr std::string_view STRING_URL = _GLOBED_STRURL;
    constexpr std::string_view STRING_WHITESPACE = _GLOBED_STRWHITESPACE;

    // If `target` is false, returns false. If `target` is true, modifies `target` to false and returns true.
    bool swapFlag(bool& target);

    // Like `swapFlag` but for optional types
    template <typename T>
    std::optional<T> swapOptional(std::optional<T>& val) {
        if (!val.has_value()) {
            return std::nullopt;
        }

        T copy = val.value();
        val.reset();
        return copy;
    }

    // Utility function for converting various enums between each other, is specialized separately for different enums
    template <typename To, typename From>
    To convertEnum(From value);

    // On first call, simply calls `func`. On repeated calls, given the same `key`, the function will not be called again. Not thread safe.
    void callOnce(const char* key, std::function<void()> func);

    // Same as `callOnce` but guaranteed to be thread-safe. Though, if another place ever calls the non-safe version, no guarantees are made.
    // When using this, it is recommended that the function does not take long to execute, due to the simplicity of the implementation.
    void callOnceSync(const char* key, std::function<void()> func);

    // Used for doing something once. `Fuse<unique-number>::tripped()` is guaranteed to evaluate to true only once. (no guarantees if accessed from multiple threads)
    template <uint32_t FuseId>
    class Fuse {
        static bool tripped() {
            static bool trip = false;
            if (!trip) {
                trip = true;
                return false;
            }

            return true;
        }
    };

    template <typename Ret, typename Func>
    concept OnceCellFunction = requires(Func func) {
        { std::is_invocable_r_v<Ret, Func> };
    };

    // On first call to `getOrInit`, create the value with the given initializer. On next calls, just return the created value.
    template <typename Ret>
    class OnceCell {
    public:
        OnceCell() {}

        template <typename Initer>
        requires OnceCellFunction<Ret, Initer>
        Ret& getOrInit(const Initer& initer) {
            static Ret val = initer();
            return val;
        }
    };

    // Calculate the avergae volume of pcm samples, picking the fastest implementation
    float calculatePcmVolume(const float* pcm, size_t samples);

    float pcmVolumeSlow(const float* pcm, size_t samples);

    bool compareName(const std::string_view name1, const std::string_view name2);

    bool isEditorCollabLevel(LevelId levelId);
}