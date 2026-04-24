#pragma once
#include <qunet/buffers/HeapByteWriter.hpp>
#include <qunet/buffers/ByteReader.hpp>
#include <unordered_map>
#include <asp/ptr/BoxedString.hpp>

namespace globed {

struct EventDictionary {
    std::vector<uint8_t> data;
    std::unordered_map<uint32_t, asp::BoxedString> mapping;

    std::optional<asp::BoxedString> lookup(uint32_t id) const {
        auto it = mapping.find(id);
        if (it != mapping.end()) {
            return it->second;
        }
        return std::nullopt;
    }
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
