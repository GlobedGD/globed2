#include "debugging.hpp"

#include <util/formatting.hpp>
#include <util/misc.hpp>

#ifdef GEODE_IS_WINDOWS
# include <dbghelp.h>
# pragma comment(lib, "dbghelp.lib")
#endif

namespace util::debugging {
    time::micros Benchmarker::end(const std::string_view id) {
        auto idstr = std::string(id);
        auto micros = time::as<time::micros>(time::now() - _entries[idstr]);
        _entries.erase(idstr);

        return micros;
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

            geode::log::debug("[DW] {} modified - {}, hexdump: {}", key, modified, hexDumpAddress(value.address, value.size));
        }
    }

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
            geode::log::debug("Average bytes per packet: {}", formatting::formatBytes((uint64_t)bytesPerPacket));

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
        geode::log::error("suicide called at {}, terminating.", sourceLocation(loc));
		geode::log::error("If you see this, something very, very bad happened.");
        GLOBED_SUICIDE
    }
#else
    std::string sourceLocation() {
        return "unknown file sorry";
    }

    [[noreturn]] void suicide() {
        geode::log::error("suicide called at <unknown location>, terminating.");
        geode::log::error("If you see this, something very, very bad happened.");
        GLOBED_SUICIDE
    }
#endif

    void timedLog(const std::string_view message) {
        geode::log::info("\r[{}] [Globed] {}", util::formatting::formatDateTime(util::time::now()), message);
    }

#ifdef GEODE_IS_WINDOWS
    static int exceptionDummy() {
        return EXCEPTION_EXECUTE_HANDLER;
    }
#endif

    static uintptr_t adjustPointerForMaps(uintptr_t ptr) {
#ifdef GEODE_IS_ANDROID64
        // xd
        const uint64_t mask = 0xFF00000000000000;
        return ptr & ~mask;
#else
        return ptr;
#endif
    }

    bool canReadPointer(uintptr_t address, size_t align = 1) {
        if (address < 4096) return false;
        if (address % align != 0) return false;

#ifdef GEODE_IS_ANDROID
        address = adjustPointerForMaps(address);

        static misc::OnceCell<std::unordered_map<size_t, ProcMapEntry>> _maps;
        auto& maps = _maps.getOrInit([] {
            std::unordered_map<size_t, ProcMapEntry> entries;

            std::ifstream maps("/proc/self/maps");

            std::string line;
            while (std::getline(maps, line, '\n')) {
                geode::log::debug("{}", line);
                size_t spacePos = line.find(' ');
                auto addressRange = line.substr(0, spacePos);

                size_t dashPos = addressRange.find('-');
                if (dashPos == std::string::npos || dashPos == 0) continue;

                std::string baseStr = addressRange.substr(0, dashPos);
                std::string endStr = addressRange.substr(dashPos + 1);

                uintptr_t base = std::strtoul(baseStr.c_str(), nullptr, 16);
                ptrdiff_t size = std::strtoul(endStr.c_str(), nullptr, 16) - base;

                auto worldReadable = line[spacePos + 1] == 'r';

                entries.emplace(base, ProcMapEntry {
                    .size = size,
                    .readable = worldReadable,
                });

            }

            return entries;
        });

        for (const auto& [base, entry] : maps) {
            if (!(address > base && address - base < entry.size)) continue;
            return entry.readable;
        }

        return false;
#elif defined(GEODE_IS_WINDOWS)
        bool isBad = IsBadReadPtr((void*)address, 4);
        if (isBad) return false;
        return true;

        __try {
            (void) *(volatile char*)(address);
            return true;
        } __except(exceptionDummy()) {
            return false;
        }
#endif
    }

#ifdef GEODE_IS_ANDROID
    struct Typeinfo {
        void* _unkptr;
        const char* namePtr;
    };
#elif defined(GEODE_IS_WINDOWS)
    struct TypeDescriptor {
        void *pVFTable, *spare;
        char name[];
    };

    struct Typeinfo {
        unsigned int _unk1, _unk2, _unk3;
        TypeDescriptor* descriptor;
    };
#endif

    std::string getTypename(void* address) {
        // geode::log::debug("getting typename of {:X}", (uintptr_t)address - geode::base::get());
        if (!canReadPointer((uintptr_t)address, 4)) return "<Unknown, invalid address>";

        void* vtablePtr = *(void**)(address);
        return getTypenameFromVtable(vtablePtr);
    }

    std::string getTypenameFromVtable(void* address) {
        if (!canReadPointer((uintptr_t)address, 4)) return "<Unknown, invalid vtable>";

        Typeinfo** typeinfoPtrPtr = (Typeinfo**)((uintptr_t)address - sizeof(void*));
        if (!canReadPointer((uintptr_t)typeinfoPtrPtr, 4)) return "<Unknown, invalid typeinfo>";
        Typeinfo* typeinfoPtr = *typeinfoPtrPtr;
        if (!canReadPointer((uintptr_t)typeinfoPtr, 4)) return "<Unknown, invalid typeinfo>";
        Typeinfo typeinfo = *typeinfoPtr;

#ifdef GEODE_IS_ANDROID
        const char* namePtr = typeinfo.namePtr;
        // geode::log::debug("name ptr: {:X}", (uintptr_t)namePtr - geode::base::get());
        if (!canReadPointer((uintptr_t)namePtr, 4)) return "<Unknown, invalid class name>";

        int status;
        std::string demangled = abi::__cxa_demangle(namePtr, nullptr, nullptr, &status);
        if (status != 0) {
            return "<Unknown, demangle failed>";
        }

        return demangled;
#elif defined(GEODE_IS_WINDOWS)

        if (!canReadPointer((uintptr_t)typeinfo.descriptor, 4)) return "<Unknown, invalid descriptor>";
        const char* namePtr = typeinfo.descriptor->name;

        if (!canReadPointer((uintptr_t)namePtr)) return "<Unknown, invalid class name>";

        std::string demangledName;

        if (!namePtr || namePtr[0] == '\0' || namePtr[1] == '\0') {
            demangledName = "<Unknown>";
        } else {
            char demangledBuf[256];
            size_t written = UnDecorateSymbolName(namePtr + 1, demangledBuf, 256, UNDNAME_NO_ARGUMENTS);
            if (written == 0) {
                demangledName = "<Unknown>";
            } else {
                demangledName = std::string(demangledBuf, demangledBuf + written);
            }
        }

        return demangledName; // TODO
#endif
    }

    static bool likelyFloat(uint32_t bits) {
        float value = util::data::bit_cast<float>(bits);
        float absv = std::abs(value);

        return std::isfinite(value) && absv <= 100000.f && absv > 0.001f;
    }

    void dumpStruct(void* address, size_t size) {
        geode::log::debug("Struct {}", getTypename(address));
        for (uintptr_t node = 0; node < size; node += sizeof(void*)) {
            uintptr_t nodeValue = *(uintptr_t*)((uintptr_t)address + node);

            auto name = getTypename((void*)nodeValue);

#if UINTPTR_MAX > 0xffffffff
            std::string prefix = fmt::format("0x{:X} : {:016X}", node, nodeValue);
#else
            std::string prefix = fmt::format("0x{:X} : {:08X}", node, nodeValue);
#endif

            // valid type with a known typeinfo
            if (!name.starts_with("<Unknown")) {
                geode::log::debug("{} ({}*)", prefix, name);
                continue;
            }

            // vtable ptr
            name = getTypenameFromVtable((void*)nodeValue);
            if (!name.starts_with("<Unknown")) {
                geode::log::debug("{} (vtable for {})", prefix, name);
                continue;
            }

            // float?
#if UINTPTR_MAX > 0xffffffff
            uint32_t lowerValue = (uint32_t)(nodeValue & 0xffffffff);
            uint32_t higherValue = (uint32_t)((nodeValue >> 32) & 0xffffffff);

            // i hate endianness
            bool float1 = likelyFloat(lowerValue), float2 = likelyFloat(higherValue);
            if (float1 && float2) {
                geode::log::debug("{} ({}f and {}f)", prefix, util::data::bit_cast<float>(lowerValue), util::data::bit_cast<float>(higherValue));
                continue;
            } else if (float1 && !float2) {
                geode::log::debug("{} ({} and {}f)", prefix, util::data::bit_cast<float>(lowerValue), higherValue);
                continue;
            } else if (!float1 && float2) {
                geode::log::debug("{} ({}f and {})", prefix, lowerValue, util::data::bit_cast<float>(higherValue));
                continue;
            }
#else
            if (likelyFloat(nodeValue)) {
                geode::log::debug("{} ({}f)", prefix, util::data::bit_cast<float>(nodeValue));
                continue;
            }
#endif

            // valid heap pointer
            if (canReadPointer(nodeValue)) {
                geode::log::debug("{} ({}) (ptr)", prefix, nodeValue);
                continue;
            }

            // anything tbh
            geode::log::debug("{} ({})", prefix, nodeValue);
        }
    }
}
