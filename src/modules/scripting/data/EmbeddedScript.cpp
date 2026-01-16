#include "EmbeddedScript.hpp"
#include <Geode/Geode.hpp>
#include <globed/util/format.hpp>
#include <qunet/buffers/ByteReader.hpp>
#include <qunet/buffers/HeapByteWriter.hpp>
#include <qunet/compression/ZstdCompressor.hpp>
#include <qunet/compression/ZstdDecompressor.hpp>

using namespace geode::prelude;

static constexpr std::array EMBEDDED_SCRIPT_MAGIC = {'\xc4', '\x19', '\x7b', '\xfa'};

static bool isEmbeddedScript(const gd::string &str)
{
    if (str.size() < EMBEDDED_SCRIPT_MAGIC.size()) {
        return false;
    }

    for (size_t i = 0; i < EMBEDDED_SCRIPT_MAGIC.size(); i++) {
        if (EMBEDDED_SCRIPT_MAGIC[i] != str[i])
            return false;
    }

    return true;
}

#define MAP_UNWRAP(x) GEODE_UNWRAP((x).mapErr([](const auto &e) { return e.message(); }))

namespace globed {

Result<EmbeddedScript> EmbeddedScript::decode(std::span<const uint8_t> data)
{
    EmbeddedScript script{};

    auto offsetRes = decodeHeader(data);
    if (!offsetRes) {
        return Err("Failed to parse embedded script header");
    }

    auto offset = *offsetRes;
    data = data.subspan(offset);

    qn::ByteReader reader{data};
    size_t decompressedLen = MAP_UNWRAP(reader.readU32());
    data = data.subspan(4);

    if (decompressedLen > 512 * 1024) {
        return Err("not decoding a script over 512kib: {} bytes", decompressedLen);
    }

    auto decompressedBuf = std::make_unique<uint8_t[]>(decompressedLen);
    MAP_UNWRAP(qn::decompressZstd(data.data(), data.size(), decompressedBuf.get(), decompressedLen));
    log::debug("Decompressed: {}", hexEncode(decompressedBuf.get(), decompressedLen));

    reader = qn::ByteReader{decompressedBuf.get(), decompressedLen};
    script.main = MAP_UNWRAP(reader.readBool());
    script.filename = MAP_UNWRAP(reader.readStringU16());
    script.content = MAP_UNWRAP(reader.readStringU32());

    if (MAP_UNWRAP(reader.readBool())) {
        script.signature = std::array<uint8_t, 32>{};
        MAP_UNWRAP(reader.readBytes(script.signature->data(), 32));
    }

    return Ok(std::move(script));
}

std::optional<size_t> EmbeddedScript::decodeHeader(std::span<const uint8_t> data)
{
    // scan until a null byte is found
    auto it = data.begin();

    while (it != data.end() && *it != '\0') {
        it++;
    }

    if (it == data.end()) {
        return std::nullopt;
    }

    // skip null byte
    it++;

    size_t distance = std::distance(it, data.end());
    if (distance < EMBEDDED_SCRIPT_MAGIC.size()) {
        return std::nullopt;
    }

    for (auto m : EMBEDDED_SCRIPT_MAGIC) {
        if ((uint8_t)m != *it) {
            return std::nullopt;
        }

        it++;
    }

    return std::distance(data.begin(), it);
}

Result<std::vector<uint8_t>> EmbeddedScript::encode(bool includePrefix, std::string_view prefixStr) const
{
    qn::HeapByteWriter writer;
    writer.writeBool(this->main);
    MAP_UNWRAP(writer.writeStringU16(this->filename));
    MAP_UNWRAP(writer.writeStringU32(this->content));
    writer.writeBool(this->signature.has_value());

    if (this->signature) {
        writer.writeBytes(*signature);
    }

    auto rawData = writer.written();
    size_t bound = qn::ZstdCompressor::compressBound(rawData.size());

    qn::HeapByteWriter outWriter;
    if (includePrefix) {
        outWriter.writeBytes((uint8_t *)prefixStr.data(), prefixStr.size());
        outWriter.writeU8(0);
        outWriter.writeBytes((uint8_t *)EMBEDDED_SCRIPT_MAGIC.data(), EMBEDDED_SCRIPT_MAGIC.size());
    }

    outWriter.writeU32(rawData.size()); // uncompressed size

    auto lastPos = outWriter.position();
    outWriter.writeZeroes(bound);
    auto fullWritten = outWriter.written();

    // rn fullWritten may or may not contain the null byte and magic, offset to get the actual compress destination
    auto compressDest = fullWritten.subspan(lastPos);

    MAP_UNWRAP(qn::compressZstd(rawData.data(), rawData.size(), const_cast<uint8_t *>(compressDest.data()), bound, 8));

    auto outVector = std::move(outWriter).intoVector();

    // truncate extra data
    outVector.resize(lastPos + bound);

    return Ok(std::move(outVector));
}

} // namespace globed

#include <Geode/utils/base64.hpp>
$on_mod(Loaded)
{
    // Encode a blank script
    globed::EmbeddedScript scr{};
    scr.filename = "script.lua";
    scr.content = "-- Your code here";
    scr.main = false;
    auto contents = scr.encode(true, "GLOBED_SCRIPT").unwrap();
    auto encoded = geode::utils::base64::encode(contents);
    log::debug("{}", encoded);

    auto test = "VEVTVF9TQ1JJUFQAxBl7-lIAAAAotS_"
                "9IFItAgB0AwAJAGhlbGxvLmx1YUEAAABmdW5jdGlvbiBfd29ybGQoKQoJcHJpbnQoIkgsICEiKQplbmQKCgAEAE7d0XkYsFC2Ew8=";
    auto data = geode::utils::base64::decode(test, base64::Base64Variant::UrlWithPad).unwrap();

    auto res = globed::EmbeddedScript::decode(std::span{data.begin(), data.end()});
    if (!res) {
        log::error("Failed: {}", res.unwrapErr());
        return;
    }

    auto script = std::move(res).unwrap();
    log::debug("- filename: {}", script.filename);
    log::debug("- main: {}", script.main);
    log::debug("- has signature: {}", script.signature.has_value());
    log::debug("- content: {}", script.content);
}