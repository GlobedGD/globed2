#include "debugging.hpp"

#include <util/formatting.hpp>

namespace util::debugging {
    time::micros Benchmarker::end(std::string id) {
        auto micros = time::as<time::micros>(time::now() - _entries[id]);
        _entries.erase(id);

        return micros;
    }

    GLOBED_SINGLETON_DEF(PacketLogger)

    void PacketLogSummary::print() {
        geode::log::debug("====== Packet summary ======");
        if (total == 0) {
            geode::log::debug("No packets have been sent during this period.");
        } else {
            geode::log::debug("Total packets: {} ({} sent, {} received)", total, totalOut, totalIn);
            geode::log::debug("Encrypted packets: {} ({} cleartext, ratio: {}%)", totalEncrypted, totalCleartext, encryptedRatio * 100);
            geode::log::debug(
                "Total bytes transferred: {} ({} sent, {} received)",
                formatting::formatBytes(totalBytes),
                formatting::formatBytes(totalBytesOut),
                formatting::formatBytes(totalBytesIn)
            );
            geode::log::debug("Average bytes per packet: {}", formatting::formatBytes(bytesPerPacket));

            // sort packets by the counts
            std::vector<std::pair<packetid_t, size_t>> pc(packetCounts.begin(), packetCounts.end());
            std::sort(pc.begin(), pc.end(), [](const auto& a, const auto& b) {
                return a.second > b.second;
            });

            for (const auto& [id, count] : pc) {
                geode::log::debug("Packet {} - {} occurrences", id, count);
            }
        }
        geode::log::debug("==== Packet summary end ====");
    }

    PacketLogSummary PacketLogger::getSummary() {
        PacketLogSummary summary = {};

        for (auto& log : queue.extract()) {
            summary.total++;

            summary.totalBytes += log.bytes;
            if (log.outgoing) {
                summary.totalOut++;
                summary.totalBytesOut += log.bytes;
            } else {
                summary.totalIn++;
                summary.totalBytesIn += log.bytes;
            }

            log.encrypted ? summary.totalEncrypted++ : summary.totalCleartext++;

            if (!summary.packetCounts.contains(log.id)) {
                summary.packetCounts[log.id] = 0;
            }

            summary.packetCounts[log.id]++;
        }

        summary.bytesPerPacket = (float)summary.totalBytes / summary.total;
        summary.encryptedRatio = (float)summary.totalEncrypted / summary.total;

        return summary;
    }

    std::string hexDumpAddress(uintptr_t addr, size_t bytes) {
        unsigned char* ptr = reinterpret_cast<unsigned char*>(addr);

        std::stringstream ss;

        for (size_t i = 0; i < bytes; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(i[ptr]);
        }

        return ss.str();
    }

    std::string hexDumpAddress(void* ptr, size_t bytes) {
        return hexDumpAddress(reinterpret_cast<uintptr_t>(ptr), bytes);
    }

#if GLOBED_CAN_USE_SOURCE_LOCATION
    std::string sourceLocation(const std::source_location loc) {
        return
            std::string(loc.file_name())
            + ":"
            + std::to_string(loc.line())
            + " ("
            + std::string(loc.function_name())
            + ")";
    }

    [[noreturn]] void suicide(const std::source_location loc) {
        geode::log::error("suicide called at " + sourceLocation(loc) + ", terminating.");
		geode::log::error("If you see this, something very, very bad happened.");
        GLOBED_SUICIDE;
    }
#else
    std::string sourceLocation() {
        return "unknown file sorry";
    }

    [[noreturn]] void suicide() {
        GLOBED_REQUIRE_LOG("suicide called at <unknown location>, terminating.");
        GLOBED_REQUIRE_LOG("If you see this, something very, very bad happened.");
        GLOBED_SUICIDE;
    }
#endif

    void timedLog(const std::string& message) {
        geode::log::info("\r[{}] [Globed] {}", util::formatting::formatDateTime(util::time::now()), message);
    }
}