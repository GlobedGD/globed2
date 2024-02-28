#include "debug.hpp"

#if defined(GEODE_IS_ANDROID)
# include <cxxabi.h>
#elif defined(GEODE_IS_WINDOWS)
# include <dbghelp.h>
# pragma comment(lib, "dbghelp.lib")
#endif

#include <any>

#include <util/format.hpp>
#include <util/rng.hpp>

using namespace geode::prelude;

namespace util::debug {
    void Benchmarker::start(const std::string_view id) {
        _entries[std::string(id)] = time::now();
    }

    time::micros Benchmarker::end(const std::string_view id) {
        auto idstr = std::string(id);
        auto micros = time::as<time::micros>(time::now() - _entries[idstr]);
        _entries.erase(idstr);

        return micros;
    }

    void Benchmarker::endAndLog(const std::string_view id, bool ignoreSmall) {
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

    void Benchmarker::runAndLog(std::function<void()>&& func, const std::string_view identifier) {
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
        GLOBED_SUICIDE
    }
#else
    std::string sourceLocation() {
        return "unknown file sorry";
    }

    [[noreturn]] void suicide() {
        log::error("suicide called at <unknown location>, terminating.");
        log::error("If you see this, something very, very bad happened.");
        GLOBED_SUICIDE
    }
#endif

    void delayedSuicide(const std::string_view message) {
        Loader::get()->queueInMainThread([message = std::string(message)] {
            log::error("Globed fatal error: {}", message);
            throw std::runtime_error(fmt::format("Globed error: {}", message));
        });
    }

    void timedLog(const std::string_view message) {
        log::info("\r[{}] [Globed] {}", util::format::formatDateTime(util::time::systemNow()), message);
    }

#ifdef GEODE_IS_WINDOWS
    static int exceptionDummy() {
        return EXCEPTION_EXECUTE_HANDLER;
    }
#endif

    static uintptr_t adjustPointerForMaps(uintptr_t ptr) {
#ifdef GEODE_IS_ANDROID64
        // remove the tag (msb)
        const uint64_t mask = 0xFF00000000000000;
        return ptr & ~mask;
#else
        return ptr;
#endif
    }

    static uintptr_t isPointerTagged(uintptr_t ptr) {
#ifdef GEODE_IS_ANDROID64
        return (ptr >> 56) == 0xb4;
#else
        return true;
#endif
    }

    bool canReadPointer(uintptr_t address, size_t align = 1) {
        if (address < 0x1000) return false;
        if (address % align != 0) return false;

#ifdef GLOBED_IS_UNIX
        address = adjustPointerForMaps(address);

        static misc::OnceCell<std::map<size_t, ProcMapEntry>> _maps;
        auto& maps = _maps.getOrInit([] {
            auto start = util::time::now();

            std::map<size_t, ProcMapEntry> entries;

            std::ifstream maps("/proc/self/maps");

            std::string line;
            while (std::getline(maps, line, '\n')) {
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

            log::debug("took {} to parse proc maps", util::format::formatDuration(util::time::now() - start));

            return entries;
        });

        for (const auto& [base, entry] : maps) {
            if (!(address > base && address - base < entry.size)) continue;
            return entry.readable;
        }

        return false;
#elif defined(GEODE_IS_WINDOWS)
        bool isBad = IsBadReadPtr((void*)address, align);
        if (isBad) return false;
        return true;

        // this one doesnt work
        __try {
            (void) *(volatile char*)(address);
            return true;
        } __except(exceptionDummy()) {
            return false;
        }
#endif
    }

#ifdef GLOBED_IS_UNIX
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

    Result<std::string> getTypename(void* address) {
        if (!canReadPointer((uintptr_t)address, 4)) return Err("invalid address");

        void* vtablePtr = *(void**)(address);
        return getTypenameFromVtable(vtablePtr);
    }

    Result<std::string> getTypenameFromVtable(void* address) {
        if (!canReadPointer((uintptr_t)address, 4)) return Err("invalid vtable");

        Typeinfo** typeinfoPtrPtr = (Typeinfo**)((uintptr_t)address - sizeof(void*));
        if (!canReadPointer((uintptr_t)typeinfoPtrPtr, 4)) return Err("invalid typeinfo");
        Typeinfo* typeinfoPtr = *typeinfoPtrPtr;
        if (!canReadPointer((uintptr_t)typeinfoPtr, 4)) return Err("invalid typeinfo");
        Typeinfo typeinfo = *typeinfoPtr;

#ifdef GLOBED_IS_UNIX
        const char* namePtr = typeinfo.namePtr;
        // log::debug("name ptr: {:X}", (uintptr_t)namePtr - geode::base::get());
        if (!canReadPointer((uintptr_t)namePtr, 4)) return Err("invalid class name");

        int status;
        char* demangledBuf = abi::__cxa_demangle(namePtr, nullptr, nullptr, &status);
        if (status != 0) {
            return Err("demangle failed");
        }

        std::string demangled(demangledBuf);
        free(demangledBuf);

        return Ok(demangled);
#elif defined(GEODE_IS_WINDOWS)

        if (!canReadPointer((uintptr_t)typeinfo.descriptor, 4)) return Err("invalid descriptor");
        const char* namePtr = typeinfo.descriptor->name;

        if (!canReadPointer((uintptr_t)namePtr)) return Err("invalid class name");

        // TODO windows: check if the name is proper

        if (!namePtr || namePtr[0] == '\0' || namePtr[1] == '\0') {
            return Err("failed to demangle");
        } else {
            char demangledBuf[256];
            size_t written = UnDecorateSymbolName(namePtr + 1, demangledBuf, 256, UNDNAME_NO_ARGUMENTS);
            if (written == 0) {
                return Err("failed to demangle");
            } else {
                return Ok(std::string(demangledBuf, demangledBuf + written));
            }
        }
#endif
    }

    static bool likelyFloat(uint32_t bits) {
        float value = util::data::bit_cast<float>(bits);
        float absv = std::abs(value);

        return std::isfinite(value) && absv <= 100000.f && absv > 0.001f;
    }

    static bool likelyDouble(uint64_t bits) {
        double value = util::data::bit_cast<double>(bits);
        double absv = std::abs(value);

        return std::isfinite(value) && absv <= 1000000.0 && absv > 0.0001;
    }

    static bool likelySeedValue(uint32_t val1, uint32_t val2, uint32_t val3) {
        size_t invalids = 0;
        invalids += val1 == 0 | val1 == 0xffffffff;
        invalids += val2 == 0 | val2 == 0xffffffff;
        invalids += val3 == 0 | val3 == 0xffffffff;

        if (invalids > 1) return false;

        return val1 + val2 == val3 || val1 + val3 == val2 || val2 + val3 == val1;
    }

    static bool likelyString(uintptr_t address) {
        // cursed code
        const unsigned char* data = (unsigned char*)address;

        size_t asciiBytes = 0;
        while (*data != '\0') {
            if (*data <= 127) {
                asciiBytes++;
            } else break;

            data++;
        };

        return asciiBytes > 2 && data[asciiBytes] == '\0';
    }

    enum class ScanItemType {
        Float, Double, SeedValue, HeapPointer, String, EmptyString
    };

#ifdef GEODE_IS_ANDROID
    uintptr_t getEmptyString() {
        static misc::OnceCell<uintptr_t> _internalstr;
        return _internalstr.getOrInit([] {
            // thank you matcool from run info
            auto* cc = new CCString();
            uintptr_t address = (uintptr_t)*(const char**) &cc->m_sString;
            delete cc;
            return address;
        });
    }
#endif

    static std::unordered_map<ptrdiff_t, ScanItemType> scanMemory(void* address, size_t size) {
        // 4-byte pass
        std::unordered_map<ptrdiff_t, ScanItemType> out;
        for (ptrdiff_t node = 0; node < size; node += 4) {
            uint32_t nodeValue = *(uint32_t*)((uintptr_t)address + node);
            if (likelyFloat(nodeValue)) {
                out.emplace(node, ScanItemType::Float);
                continue;
            }

#if UINTPTR_MAX <= 0xffffffff
# ifdef GEODE_IS_ANDROID
            if (getEmptyString() == nodeValue) {
                out.emplace(node, ScanItemType::String);
                continue;
            } else if (canReadPointer((uintptr_t)nodeValue)) {
                if (likelyString((uintptr_t)nodeValue)) {
                    out.emplace(node, ScanItemType::String);
                } else {
                    out.emplace(node, ScanItemType::HeapPointer);
                }
                continue;
            }
# else // GEODE_IS_ANDROID
            if (canReadPointer((uintptr_t)nodeValue)) {
                if (likelyString((uintptr_t)nodeValue)) {
                    out.emplace(node, ScanItemType::String);
                } else {
                    out.emplace(node, ScanItemType::HeapPointer);
                }
                continue;
            }
# endif // GEODE_IS_ANDROID
#endif
        }

        // 8-byte pass
        for (ptrdiff_t node = 0; node < size; node += 4) {
            uint64_t nodeValue = *(uint64_t*)((uintptr_t)address + node);
            if (nodeValue % alignof(double) == 0 && !out.contains(node) && !out.contains(node + 4) && likelyDouble(nodeValue)) {
                out.emplace(node, ScanItemType::Double);
                continue;
            }

#if UINTPTR_MAX > 0xffffffff
            if (node % alignof(void*) == 0) {
# ifdef GEODE_IS_ANDROID
                if (getEmptyString() == nodeValue) {
                    out.emplace(node, ScanItemType::EmptyString);
                    continue;
                } else if (canReadPointer((uintptr_t)nodeValue)) {
                    if (likelyString((uintptr_t)nodeValue)) {
                        out.emplace(node, ScanItemType::String);
                    } else {
                        out.emplace(node, ScanItemType::HeapPointer);
                    }
                    continue;
                }
# else // GEODE_IS_ANDROID
                if (canReadPointer((uintptr_t)nodeValue)) {
                    if (likelyString((uintptr_t)nodeValue)) {
                        out.emplace(node, ScanItemType::String);
                    } else {
                        out.emplace(node, ScanItemType::HeapPointer);
                    }
                    continue;
                }
# endif // GEODE_IS_ANDROID
            }
#endif // UINTPTR_MAX > 0xffffffff
        }

        // seed value pass
        for (ptrdiff_t node = 8; node < size; node += 4) {
            uint32_t nodeValue1 = *(uint32_t*)((uintptr_t)address + node - 8);
            uint32_t nodeValue2 = *(uint32_t*)((uintptr_t)address + node - 4);
            uint32_t nodeValue3 = *(uint32_t*)((uintptr_t)address + node);

            if (!out.contains(node - 8) && !out.contains(node - 4) && !out.contains(node) && likelySeedValue(nodeValue1, nodeValue2, nodeValue3)) {
                out.emplace(node - 8, ScanItemType::SeedValue);
                node += 8;
            }
        }

        return out;
    }

    void dumpStruct(void* address, size_t size) {
        auto typenameResult = getTypename(address);

        if (typenameResult.isErr()) {
            log::warn("Failed to dump struct: {}", typenameResult.unwrapErr());
            return;
        }

        log::debug("Struct {}", typenameResult.unwrap());
        auto scanResult = scanMemory(address, size);

        const size_t INCREMENT = 4;
        for (uintptr_t node = 0; node < size; node += INCREMENT) {
            uint32_t nodeValue32 = *(uint32_t*)((uintptr_t)address + node);
            uint64_t nodeValue64 = *(uint64_t*)((uintptr_t)address + node);
            uintptr_t nodeValuePtr = *(uintptr_t*)((uintptr_t)address + node);

            std::string prefix32 = fmt::format("0x{:X} : {:08X}", node, nodeValue32);
            std::string prefix64 = fmt::format("0x{:X} : {:016X}", node, nodeValue64);
            std::string& prefixPtr = sizeof(void*) == 4 ? prefix32 : prefix64;

            if (node % alignof(void*) == 0 && canReadPointer(nodeValuePtr)) {
                auto name = getTypename((void*)nodeValuePtr);

                // valid type with a known typeinfo
                if (name.isOk()) {
                    log::debug("{} ({}*)", prefixPtr, name.unwrap());
                    node += sizeof(void*) - INCREMENT;
                    continue;
                }

                // vtable ptr
                name = getTypenameFromVtable((void*)nodeValuePtr);
                if (name.isOk()) {
                    log::debug("{} (vtable for {})", prefixPtr, name.unwrap());
                    node += sizeof(void*) - INCREMENT;
                    continue;
                }
            }

            // pre analyzed stuff
            if (scanResult.contains(node)) {
                auto type = scanResult.at(node);
                switch (type) {
                case ScanItemType::Float:
                    log::debug("{} ({}f)", prefix32, data::bit_cast<float>(nodeValue32));
                    break;
                case ScanItemType::Double:
                    log::debug("{} ({}d)", prefix64, data::bit_cast<double>(nodeValue64));
                    node += sizeof(double) - INCREMENT;
                    break;
                case ScanItemType::HeapPointer:
                    log::debug("{} ({}) (ptr)", prefixPtr, nodeValuePtr);
                    node += sizeof(void*) - INCREMENT;
                    break;
                case ScanItemType::String:
                    log::debug("{} ({}) (string: \"{}\")", prefixPtr, nodeValuePtr, (const char*)nodeValuePtr);
                    node += sizeof(void*) - INCREMENT;
                    break;
                case ScanItemType::EmptyString:
                    log::debug("{} ({}) (string: \"\")", prefixPtr, nodeValuePtr);
                    node += sizeof(void*) - INCREMENT;
                    break;
                case ScanItemType::SeedValue: {
                    uint32_t valueNext = *(uint32_t*)((uintptr_t)address + node + 4);
                    uint32_t valueNext2 = *(uint32_t*)((uintptr_t)address + node + 8);
                    std::string prefix96 = fmt::format("0x{:X} : {:024X}", node, nodeValue64);
                    log::debug("0x{:X} : ({:08X} {:08X} {:08X}) seed value: {}, {}, {}", node, nodeValue32, valueNext, valueNext2, nodeValue32, valueNext, valueNext2);
                    node += 3*4 - INCREMENT;
                    break;
                }
                }

                continue;
            }

            // anything tbh
            log::debug("{} ({})", prefix32, nodeValue32);
        }
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
