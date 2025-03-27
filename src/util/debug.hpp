#pragma once
#include <unordered_map>

#include <asp/sync.hpp>
#include <asp/time/Instant.hpp>

#include <data/packets/packet.hpp>
#include <util/collections.hpp>
#include <util/misc.hpp>
#include <util/singleton.hpp>

namespace util::debug {
    class Benchmarker : public SingletonBase<Benchmarker> {
    public:
        void start(std::string_view id);
        void endAndLog(std::string_view id, bool ignoreSmall = false);
        asp::time::Duration end(std::string_view id);
        asp::time::Duration run(std::function<void()>&& func);
        void runAndLog(std::function<void()>&& func, std::string_view identifier);

    private:
        std::unordered_map<std::string, asp::time::Instant> _entries;
    };

    class DataWatcher : public SingletonBase<DataWatcher> {
    public:
        struct WatcherEntry {
            uintptr_t address;
            size_t size;
            data::bytevector lastData;
        };

        void start(std::string_view id, uintptr_t address, size_t size) {
            auto idstr = std::string(id);
            _entries.emplace(std::string(idstr), WatcherEntry {
                .address = address,
                .size = size,
                .lastData = data::bytevector(size)
            });

            this->updateLastData(_entries.at(idstr));
        }

        void start(std::string_view id, void* address, size_t size) {
            this->start(id, (uintptr_t)address, size);
        }

        // returns the indexes of bytes that were modified since last read
        std::vector<size_t> updateLastData(WatcherEntry& entry);

        void updateAll();

    private:
        std::unordered_map<std::string, WatcherEntry> _entries;
    };

    struct PacketLog {
        packetid_t id;
        bool encrypted;
        bool outgoing;
        size_t bytes;
    };

    struct PacketLogSummary {
        size_t total;

        size_t totalIn;
        size_t totalOut;

        size_t totalCleartext;
        size_t totalEncrypted;

        uint64_t totalBytes;
        uint64_t totalBytesIn;
        uint64_t totalBytesOut;

        std::unordered_map<packetid_t, size_t> packetCounts;

        float bytesPerPacket;
        float encryptedRatio;

        void print();
    };

    class PacketLogger : public SingletonLeakBase<PacketLogger> {
    public:
        void record(packetid_t id, bool encrypted, bool outgoing, size_t bytes);
        PacketLogSummary getSummary();
    private:
        collections::CappedQueue<PacketLog, 25000> queue;
    };

    std::string hexDumpAddress(uintptr_t addr, size_t bytes);
    std::string hexDumpAddress(void* ptr, size_t bytes);

#if GLOBED_CAN_USE_SOURCE_LOCATION
    std::string sourceLocation(const std::source_location loc = GLOBED_SOURCE);
    // crash the program immediately, print the location of the caller
    [[noreturn]] void suicide(const std::source_location loc = GLOBED_SOURCE);
#else
    std::string sourceLocation();
    // crash the program immediately
    [[noreturn]] void suicide();
#endif

    void delayedSuicide(std::string_view);

    // like log::debug but with precise timestamps.
    void timedLog(std::string_view message);

    std::optional<ptrdiff_t> searchMember(const void* structptr, const uint8_t* bits, size_t length, size_t alignment, size_t maxSize);

    template <typename T>
    std::optional<ptrdiff_t> searchMember(const void* structptr, const T& value, size_t maxSize) {
        return searchMember(structptr, reinterpret_cast<const uint8_t*>(&value), sizeof(T), alignof(T), maxSize);
    }

    inline void logPrintfWrapper(const char* format, ...) {
        va_list args;
        va_start(args, format);

        int count = std::vsnprintf(nullptr, 0, format, args);
        std::string str(count + 1, '\0');
        std::vsnprintf(str.data(), str.size(), format, args);

        va_end(args);

        log::debug("{}", str);
    }

    template <typename T>
    void printStruct(T* s) {
#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wformat"
        __builtin_dump_struct(s, logPrintfWrapper);
# pragma clang diagnostic pop
#else
        log::warn("util::debug::printStruct not implemented for this platform");
#endif
    }

    template <typename T> requires (std::is_polymorphic_v<T>)
    std::string getTypename(T* type) {
        // TODO demangle
        return typeid(*type).name();
    }

    template <typename T>
    constexpr std::string_view getTypenameConstexpr() {
#ifdef __clang__
        constexpr auto pfx = sizeof("std::string_view getTypenameConstexpr() [T = ") - 1;
        constexpr auto sfx = sizeof("]") - 1;
        constexpr auto function = __PRETTY_FUNCTION__;
        constexpr auto len = sizeof(__PRETTY_FUNCTION__) - pfx - sfx - 1;
        return {function + pfx, len};
#else
        static_assert(false, "well well well");
#endif
    }

    bool isWine();
    const char* getWineVersion();
}

#define GLOBED_BENCH(name, ...) ::util::debug::Benchmarker().runAndLog([&] { __VA_ARGS__ ; }, name)
