#pragma once
#include <defs.hpp>
#include <unordered_map>

#include <data/packets/packet.hpp>
#include <util/collections.hpp>
#include <util/time.hpp>

namespace util::debugging {
    // To use the benchmarker, create a `Benchmarker` object, call start(id), then run all the code you want to benchmark, and call end(id).
    // It will return the number of microseconds the code took to run.
    class Benchmarker : GLOBED_SINGLETON(Benchmarker) {
    public:
        inline void start(const std::string_view id) {
            _entries.emplace(std::string(id), time::now());
        }

        time::micros end(const std::string_view id);
    private:
        std::unordered_map<std::string, time::time_point> _entries;
    };

    class DataWatcher : GLOBED_SINGLETON(DataWatcher) {
    public:
        struct WatcherEntry {
            uintptr_t address;
            size_t size;
            data::bytevector lastData;
        };

        inline void start(const std::string_view id, uintptr_t address, size_t size) {
            auto idstr = std::string(id);
            _entries.emplace(std::string(idstr), WatcherEntry {
                .address = address,
                .size = size,
                .lastData = data::bytevector(size)
            });

            this->updateLastData(_entries.at(idstr));
        }

        inline void start(const std::string_view id, void* address, size_t size) {
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

    class PacketLogger : GLOBED_SINGLETON(PacketLogger) {
    public:
        PacketLogger() {}

        inline void record(packetid_t id, bool encrypted, bool outgoing, size_t bytes) {
# ifdef GLOBED_DEBUG_PACKETS
#  ifdef GLOBED_DEBUG_PACKETS_PRINT
            geode::log::debug("{} packet {}, encrypted: {}, bytes: {}", outgoing ? "Sending" : "Receiving", id, encrypted ? "true" : "false", bytes);
#  endif // GLOBED_DEBUG_PACKETS_PRINT
            queue.push(PacketLog {
                .id = id,
                .encrypted = encrypted,
                .outgoing = outgoing,
                .bytes = bytes
            });
# endif // GLOBED_DEBUG_PACKETS
        }

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

    // like geode::log::debug but with precise timestamps.
    void timedLog(const std::string_view message);
}
