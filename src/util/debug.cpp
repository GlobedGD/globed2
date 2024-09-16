#include "debug.hpp"

#if defined(GEODE_IS_ANDROID)
# include <cxxabi.h>
#elif defined(GEODE_IS_WINDOWS)
# include <dbghelp.h>
# pragma comment(lib, "dbghelp.lib")
#endif

#include <util/format.hpp>
#include <util/rng.hpp>

using namespace geode::prelude;

namespace util::debug {
    void Benchmarker::start(std::string_view id) {
        _entries[std::string(id)] = time::now();
    }

    time::micros Benchmarker::end(std::string_view id) {
        auto idstr = std::string(id);
        auto micros = time::as<time::micros>(time::now() - _entries[idstr]);
        _entries.erase(idstr);

        return micros;
    }

    void Benchmarker::endAndLog(std::string_view id, bool ignoreSmall) {
        auto took = this->end(id);
        if (took > util::time::micros(250)) {
            log::debug("{} took {} to run", id, util::format::formatDuration(took));
        }
    }

    time::micros Benchmarker::run(std::function<void()>&& func) {
        auto& random = rng::Random::get();
        auto id = fmt::format("bb-rng-{}", random.generate<uint32_t>());
        this->start(id);
        func();
        return this->end(id);
    }

    void Benchmarker::runAndLog(std::function<void()>&& func, std::string_view identifier) {
        auto took = this->run(std::move(func));
        log::debug("{} took {} to run", identifier, util::format::formatDuration(took));
    }

    std::vector<size_t> DataWatcher::updateLastData(DataWatcher::WatcherEntry& entry) {
        std::vector<size_t> changedBytes;

        for (size_t off = 0; off < entry.size; off++) {
            auto rbyte = *(data::byte*)(entry.address + off);
            if (rbyte != entry.lastData[off]) {
                changedBytes.push_back(off);
                entry.lastData[off] = rbyte;
            }
        }

        return changedBytes;
    }

    void DataWatcher::updateAll() {
        for (auto& [key, value] : _entries) {
            auto modified = this->updateLastData(value);
            if (modified.empty()) continue;

            log::debug("[DW] {} modified - {}, hexdump: {}", key, modified, hexDumpAddress(value.address, value.size));
        }
    }

    void PacketLogSummary::print() {
        log::debug("====== Packet summary ======");
        if (total == 0) {
            log::debug("No packets have been sent during this period.");
        } else {
            log::debug("Total packets: {} ({} sent, {} received)", total, totalOut, totalIn);
            log::debug("Encrypted packets: {} ({} cleartext, ratio: {}%)", totalEncrypted, totalCleartext, encryptedRatio * 100);
            log::debug(
                "Total bytes transferred: {} ({} sent, {} received)",
                format::formatBytes(totalBytes),
                format::formatBytes(totalBytesOut),
                format::formatBytes(totalBytesIn)
            );
            log::debug("Average bytes per packet: {}", format::formatBytes((uint64_t)bytesPerPacket));

            // sort packets by the counts
            std::vector<std::pair<packetid_t, size_t>> pc(packetCounts.begin(), packetCounts.end());
            std::sort(pc.begin(), pc.end(), [](const auto& a, const auto& b) {
                return a.second > b.second;
            });

            for (const auto& [id, count] : pc) {
                log::debug("Packet {} - {} occurrences", id, count);
            }
        }
        log::debug("==== Packet summary end ====");
    }
    void PacketLogger::record(packetid_t id, bool encrypted, bool outgoing, size_t bytes) {
#ifdef GLOBED_DEBUG_PACKETS
# ifdef GLOBED_DEBUG_PACKETS_PRINT
        log::debug("{} packet {}, encrypted: {}, bytes: {}", outgoing ? "Sending" : "Receiving", id, encrypted ? "true" : "false", bytes);
# endif // GLOBED_DEBUG_PACKETS_PRINT
        queue.push(PacketLog {
            .id = id,
            .encrypted = encrypted,
            .outgoing = outgoing,
            .bytes = bytes
        });
#endif // GLOBED_DEBUG_PACKETS
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
        log::error("suicide called at {}, terminating.", sourceLocation(loc));
		log::error("If you see this, something very, very bad happened.");

        std::abort();
    }
#else
    std::string sourceLocation() {
        return "unknown file sorry";
    }

    [[noreturn]] void suicide() {
        log::error("suicide called at <unknown location>, terminating.");
        log::error("If you see this, something very, very bad happened.");
        std::abort();
    }
#endif

    void delayedSuicide(std::string_view message) {
        Loader::get()->queueInMainThread([message = std::string(message)] {
            log::error("Globed fatal error: {}", message);
            throw std::runtime_error(fmt::format("Globed error: {}", message));
        });
    }

    void timedLog(std::string_view message) {
        log::info("\r[{}] [Globed] {}", util::format::formatDateTime(util::time::systemNow()), message);
    }

    std::optional<ptrdiff_t> searchMember(const void* structptr, const uint8_t* bits, size_t length, size_t alignment, size_t maxSize) {
        // align the pos
        uintptr_t startPos = (reinterpret_cast<uintptr_t>(structptr) / alignment) * alignment;
        uintptr_t endPos = startPos + maxSize;

        for (uintptr_t currentPos = startPos; currentPos < endPos; currentPos += alignment) {
            uint8_t* targetBits = reinterpret_cast<uint8_t*>(currentPos);
            // compare
            bool match = true;
            for (size_t i = 0; i < length; i++) {
                if (targetBits[i] != bits[i]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                return currentPos - startPos;
            }
        }

        return std::nullopt;
    }
}
