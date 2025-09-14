#pragma once

#include <globed/prelude.hpp>
#include <globed/util/format.hpp>

#include <qunet/buffers/HeapByteWriter.hpp>
#include <qunet/buffers/ByteReader.hpp>
#include <Geode/Geode.hpp>

#define READER_UNWRAP(...) GEODE_UNWRAP((__VA_ARGS__).mapErr([&](auto&& err) { return err.message(); }));

namespace globed {

class ExtendedObjectBase : public ItemTriggerGameObject {
public:
    ExtendedObjectBase();

protected:
    void encodePayload(std::function<bool(qn::HeapByteWriter&)>&& writefn);

    inline static uint8_t computeChecksum(std::span<const uint8_t> data) {
        uint32_t sum = 0;
        for (auto byte : data) {
            sum += byte;
        }

        return ~sum & 0xff;
    }

    template <typename T, typename F> requires (std::is_convertible_v<std::invoke_result_t<F, qn::ByteReader&>, Result<T>>)
    Result<T> decodePayload(F&& readfn) {
        qn::HeapByteWriter writer; // TODO: use qn::ByteWriter when its implemented

        // could add other props if not enough space :p
        writer.writeU32(std::bit_cast<uint32_t>(m_item1Mode));
        writer.writeU32(std::bit_cast<uint32_t>(m_item2Mode));
        writer.writeU32(std::bit_cast<uint32_t>(m_resultType1));
        writer.writeU32(std::bit_cast<uint32_t>(m_resultType2));
        writer.writeU32(std::bit_cast<uint32_t>(m_roundType1));
        writer.writeU32(std::bit_cast<uint32_t>(m_roundType2));
        writer.writeU32(std::bit_cast<uint32_t>(m_signType1));
        writer.writeU32(std::bit_cast<uint32_t>(m_signType2));
        auto written = writer.written();

        log::debug("Decoding payload: {}", hexEncode(written.data(), written.size()));

        qn::ByteReader reader{written};

        auto res = readfn(reader);
        if (!res) {
            return Err("Failed to decode script object data (for {}): {}", typeid(*this).name(), res.unwrapErr());
        }

        // check the checksum (last byte)
        auto csumres = reader.readU8();
        if (!csumres) {
            return Err("Failed to read checksum from script object data (for {}): {}", typeid(*this).name(), csumres.unwrapErr().message());
        }

        auto csum = csumres.unwrap();
        auto data = written.subspan(0, reader.position() - 1);

        auto expected = this->computeChecksum(data);
        if (csum != expected) {
            return Err("Failed to validate checksum in script object data (for {}), expected {}, got {}", typeid(*this).name(), expected, csum);
        }

        return Ok(std::move(res).unwrap());
    }

    template <typename T, typename F> requires (std::is_convertible_v<std::invoke_result_t<F, qn::ByteReader&>, Result<T>>)
    std::optional<T> decodePayloadOpt(F&& readfn) {
        auto res = this->decodePayload<T>(std::forward<F>(readfn));

        if (!res) {
            log::warn("{}", res.unwrapErr());
            return std::nullopt;
        }

        return std::make_optional(std::move(res).unwrap());
    }
};

}
