#pragma once
#include <data/packets/packet.hpp>
#include <defs.hpp>
#include <chrono>
#include <unordered_map>
#include <util/collections.hpp>

namespace util::debugging {
    // To use the benchmarker, create a `Benchmarker` object, call start(id), then run all the code you want to benchmark, and call end(id).
    // It will return the number of microseconds the code took to run.
    class Benchmarker {
    public:
        Benchmarker() {}
        inline void start(std::string id) {
            _entries[id] = std::chrono::high_resolution_clock::now();
        }

        std::chrono::microseconds end(std::string id);
    private:
        std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> _entries;
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

    class PacketLogger {
    public:
        GLOBED_SINGLETON(PacketLogger)
        PacketLogger() {}

        inline void record(packetid_t id, bool encrypted, bool outgoing, size_t bytes) {
#ifdef GLOBED_DEBUG_PACKETS
            geode::log::debug("{} packet {}, encrypted: {}, bytes: {}", outgoing ? "Sending" : "Receiving", id, encrypted ? "true" : "false", bytes);
            queue.push(PacketLog {
                .id = id,
                .encrypted = encrypted,
                .outgoing = outgoing,
                .bytes = bytes
            });
#endif
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
}
