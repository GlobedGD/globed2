#pragma once
#include <qunet/buffers/HeapByteWriter.hpp>
#include <qunet/buffers/ByteReader.hpp>

namespace globed {

struct EventDictionary {
    uint32_t builtinsVersion;
    std::vector<uint8_t> data;
};

class EventEncoder {
public:
    EventEncoder();
    void registerEvent(std::string name);
    EventDictionary finalize(bool game) const;

private:
    std::unordered_set<std::string> m_events;
};

}
