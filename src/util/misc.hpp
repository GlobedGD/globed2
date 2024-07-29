#pragma once
#include <defs/essential.hpp>
#include <defs/geode.hpp>
#include <data/types/basic/either.hpp>

#include <functional>
#include <string_view>
#include <type_traits>
#include <optional>

// i hate c++
#define _GLOBED_STRNUM "1234567890"
#define _GLOBED_STRLOWER "abcdefghijklmnopqrstuvwxyz"
#define _GLOBED_STRUPPER "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define _GLOBED_STRALPHA _GLOBED_STRLOWER _GLOBED_STRUPPER
#define _GLOBED_STRALPHANUM _GLOBED_STRNUM _GLOBED_STRALPHA
#define _GLOBED_STRUSERNAME _GLOBED_STRALPHANUM " "
#define _GLOBED_STRSPECIAL "@$#&* {}[],=-().+;'/\"!%^_\\:<>?`~"
#define _GLOBED_STRPRINTABLEINPUT _GLOBED_STRALPHANUM _GLOBED_STRSPECIAL
#define _GLOBED_STRPRINTABLE _GLOBED_STRPRINTABLEINPUT "\r\t\n"
#define _GLOBED_STRURL _GLOBED_STRALPHANUM ":/%._-?#"
#define _GLOBED_STRWHITESPACE " \t\n\r\x0b\x0c"


struct PlayerIconData;
enum class PlayerIconType : uint8_t;
enum class IconType;

namespace util::misc {
    constexpr std::string_view STRING_DIGITS = _GLOBED_STRNUM;
    constexpr std::string_view STRING_LOWERCASE = _GLOBED_STRLOWER;
    constexpr std::string_view STRING_UPPERCASE = _GLOBED_STRUPPER;
    constexpr std::string_view STRING_ALPHABET = _GLOBED_STRALPHA;
    constexpr std::string_view STRING_ALPHANUMERIC = _GLOBED_STRALPHANUM;
    constexpr std::string_view STRING_USERNAME = _GLOBED_STRUSERNAME;
    constexpr std::string_view STRING_SPECIAL_CHARS = _GLOBED_STRSPECIAL;
    constexpr std::string_view STRING_PRINTABLE = _GLOBED_STRPRINTABLE;
    constexpr std::string_view STRING_PRINTABLE_INPUT = _GLOBED_STRPRINTABLEINPUT;
    constexpr std::string_view STRING_URL = _GLOBED_STRURL;
    constexpr std::string_view STRING_WHITESPACE = _GLOBED_STRWHITESPACE;

    template <typename>
    struct is_either : std::false_type {};

    template <typename T, typename Y>
    struct is_either<Either<T, Y>> : std::true_type {};

    // If `target` is false, returns false. If `target` is true, modifies `target` to false and returns true.
    bool swapFlag(bool& target);

    // Like `swapFlag` but for optional types
    template <typename T>
    std::optional<T> swapOptional(std::optional<T>& val) {
        if (!val.has_value()) {
            return std::nullopt;
        }

        T copy = std::move(val.value());
        val.reset();
        return copy;
    }

    // On first call, simply calls `func`. On repeated calls, given the same `key`, the function will not be called again. Not thread safe.
    void callOnce(const char* key, std::function<void()> func);

    // Same as `callOnce` but guaranteed to be thread-safe. Though, if another place ever calls the non-safe version, no guarantees are made.
    // When using this, it is recommended that the function does not take long to execute, due to the simplicity of the implementation.
    void callOnceSync(const char* key, std::function<void()> func);

    // Calculate the avergae volume of pcm samples, picking the fastest implementation
    float calculatePcmVolume(const float* pcm, size_t samples);

    float pcmVolumeSlow(const float* pcm, size_t samples);

    bool compareName(const std::string_view name1, const std::string_view name2);

    bool isEditorCollabLevel(LevelId levelId);

    class ScopeGuard {
    public:
        ScopeGuard(const std::function<void()>& f) : f(f) {}
        ScopeGuard(std::function<void()>&& f) : f(std::move(f)) {}

        ~ScopeGuard();

    private:
        std::function<void()> f;
    };

    ScopeGuard scopeDestructor(const std::function<void()>& f);
    ScopeGuard scopeDestructor(std::function<void()>&& f);

    class UniqueIdent {
    public:
        UniqueIdent(std::array<uint8_t, 32> data) : rawForm(data) {}

        operator std::string() const;
        std::array<uint8_t, 32> getRaw() const;
        std::string getString() const;

    private:
        std::array<uint8_t, 32> rawForm;
    };

    // Generates an identifier that should be unique per any possible device.
    const UniqueIdent& fingerprint();
    Result<UniqueIdent> fingerprintImpl(); // different definition per platform.

    // If you are reading this, feel free to trace all usages of this function.
    // The fingerprint is never sent anywhere, and is only used for local encryption.
}
