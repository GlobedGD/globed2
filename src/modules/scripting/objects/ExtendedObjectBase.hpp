#pragma once

#include <globed/prelude.hpp>
#include <globed/util/format.hpp>

#include <dbuf/ByteReader.hpp>
#include <dbuf/ByteWriter.hpp>
#include <Geode/Geode.hpp>
#include <Geode/utils/function.hpp>

namespace globed {

class ExtendedObjectBase : public ItemTriggerGameObject {
public:
    ExtendedObjectBase();

protected:
    void encodePayload(geode::FunctionRef<bool(dbuf::ByteWriter<>&)>&& writefn);

    inline static uint8_t computeChecksum(std::span<const uint8_t> data) {
        uint32_t sum = 0;
        for (auto byte : data) {
            sum += byte;
        }

        return ~sum & 0xff;
    }

    dbuf::ByteReader<> _decodePayloadPre(dbuf::ArrayByteWriter<64>& writer);
    Result<> _decodePayloadPost(dbuf::ArrayByteWriter<64>& writer, dbuf::ByteReader<>& reader);

    template <typename T, typename F> requires (std::is_convertible_v<std::invoke_result_t<F, dbuf::ByteReader<>&>, Result<T>>)
    Result<T> decodePayload(F&& readfn) {
        dbuf::ArrayByteWriter<64> writer;
        dbuf::ByteReader<> reader = this->_decodePayloadPre(writer);

        auto res = readfn(reader);
        if (!res) {
            return Err("Failed to decode script object data (for {}): {}", typeid(*this).name(), res.unwrapErr());
        }

        GEODE_UNWRAP(this->_decodePayloadPost(writer, reader));

        return Ok(std::move(res).unwrap());
    }

    template <typename T, typename F> requires (std::is_convertible_v<std::invoke_result_t<F, dbuf::ByteReader<>&>, Result<T>>)
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
