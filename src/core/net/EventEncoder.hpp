#pragma once
#include <globed/core/Event.hpp>
#include <dbuf/ByteWriter.hpp>
#include <dbuf/ByteReader.hpp>
#include <asp/ptr/BoxedString.hpp>
#include <asp/iter.hpp>

namespace globed {

struct EventFlags {
    enum class Type {
        /// This event targets specific players, specified at runtime.
        /// If this is not specified, the event is sent to everyone in the same room/session.
        TARGET_PLAYERS = 1 << 0,
        /// This event has no data.
        NO_DATA = 1 << 1,
        /// This event is reliable and must be delivered.
        RELIABLE = 1 << 2,
        /// This event is high priority and should be sent without much extra delay.
        URGENT = 1 << 3,
        /// This event was sent to us by another player
        SENT_BY_PLAYER = 1 << 4,
        /// This event should be echoed back to the player that sent it.
        SEND_BACK = 1 << 5,

        /// There is another byte that has extra flags
        EXTENDED_FLAGS = 1 << 7
    };

    using enum Type;

    EventFlags(Type val) : _val(static_cast<uint8_t>(val)) {}
    EventFlags(uint8_t val) : _val(val) {}
    EventFlags() : _val(0) {}

    operator uint8_t() const { return _val; }

    EventFlags& operator|=(EventFlags other) {
        _val |= other._val;
        return *this;
    }

    EventFlags operator&(EventFlags other) const {
        return EventFlags{ (uint8_t)(_val & other._val) };
    }

private:
    uint8_t _val;
};

struct EventDictionary;

class EventIterator : public asp::iter::Iter<EventIterator, geode::Result<RawBorrowedEvent>> {
public:
    using Item = geode::Result<RawBorrowedEvent>;

    EventIterator(std::span<const uint8_t> data, EventDictionary& dictionary);
    std::optional<Item> next();

private:
    dbuf::ByteReader<> m_reader;
    EventDictionary& m_dictionary;
    size_t m_remCount = 0;
    bool m_eof = false;
};


struct EventDictionary {
    std::vector<uint8_t> data;
    std::vector<asp::BoxedString> mapping;

    std::optional<asp::BoxedString> lookup(uint32_t id) const;
    std::optional<uint32_t> lookupId(std::string_view name) const;

    template <typename Wr>
    bool writeOne(
        dbuf::ByteWriter<Wr>& writer,
        std::string_view id,
        std::span<const uint8_t> data,
        const EventOptions& options
    ) {
        auto nid = this->lookupId(id);
        if (!nid) {
            geode::log::warn("cannot encode unknown event '{}'", id);
            return false;
        }

        size_t total = mapping.size();

        if (total < 256) {
            writer.writeU8(*nid);
        } else if (total < 65536) {
            writer.writeU16(*nid);
        } else {
            writer.writeU32(*nid);
        }

        auto flagPos = writer.position();
        writer.writeU8(0); // placeholder

        EventFlags flags{};

        if (!options.targetPlayers.empty()) {
            flags |= EventFlags::TARGET_PLAYERS;
            writer.writeVarUint(options.targetPlayers.size()).unwrap();
            for (int32_t pid : options.targetPlayers) {
                writer.writeI32(pid);
            }
        }

        if (data.empty()) {
            flags |= EventFlags::NO_DATA;
        } else {
            writer.writeVarUint(data.size()).unwrap();
            writer.writeBytes(data);
        }

        if (options.reliable) {
            flags |= EventFlags::RELIABLE;
        }
        if (options.urgent) {
            flags |= EventFlags::URGENT;
        }
        if (options.sendBack) {
            flags |= EventFlags::SEND_BACK;
        }

        writer.performAt(flagPos, [&](auto& writer) {
            writer.writeU8((uint8_t) flags);
        }).unwrap();

        return true;
    }

    /// writes events, includes the length
    template <typename Wr>
    bool writeMany(
        dbuf::ByteWriter<Wr>& writer,
        auto& events
    ) {
        writer.writeVarUint(events.size()).unwrap();
        for (const auto& event : events) {
            if (!this->writeOne(writer, event.name, event.data, event.options)) {
                return false;
            }
        }
        return true;
    }

    EventIterator decode(std::span<const uint8_t> data) {
        return EventIterator{data, *this};
    }
};

class EventEncoder {
public:
    EventEncoder();
    bool registerEvent(std::string name);
    EventDictionary finalize(bool game) const;

private:
    std::unordered_set<std::string> m_events;
};

}
