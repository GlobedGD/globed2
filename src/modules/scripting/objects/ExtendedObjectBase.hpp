#pragma once

#include <globed/prelude.hpp>
#include <globed/util/format.hpp>

#include <qunet/buffers/ArrayByteWriter.hpp>
#include <qunet/buffers/HeapByteWriter.hpp>
#include <qunet/buffers/ByteReader.hpp>
#include <Geode/Geode.hpp>
#include <std23/function_ref.h>

#define READER_UNWRAP(...) GEODE_UNWRAP((__VA_ARGS__).mapErr([&](auto&& err) { return err.message(); }));

namespace globed {

class ExtendedObjectBase : public ItemTriggerGameObject {
public:
    ExtendedObjectBase();

protected:
    void encodePayload(std23::function_ref<bool(qn::HeapByteWriter&)>&& writefn);

    inline static uint8_t computeChecksum(std::span<const uint8_t> data) {
        uint32_t sum = 0;
        for (auto byte : data) {
            sum += byte;
        }

        return ~sum & 0xff;
    }

    qn::ByteReader _decodePayloadPre(qn::ArrayByteWriter<64>& writer);
    Result<> _decodePayloadPost(qn::ArrayByteWriter<64>& writer, qn::ByteReader& reader);

    template <typename T, typename F> requires (std::is_convertible_v<std::invoke_result_t<F, qn::ByteReader&>, Result<T>>)
    Result<T> decodePayload(F&& readfn) {
        qn::ArrayByteWriter<64> writer;
        qn::ByteReader reader = this->_decodePayloadPre(writer);

        auto res = readfn(reader);
        if (!res) {
            return Err("Failed to decode script object data (for {}): {}", typeid(*this).name(), res.unwrapErr());
        }

        GEODE_UNWRAP(this->_decodePayloadPost(writer, reader));

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
