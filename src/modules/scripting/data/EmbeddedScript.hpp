#pragma once
#include <Geode/Result.hpp>
#include <span>
#include <stdint.h>
#include <string>

namespace globed {

struct EmbeddedScript {
    std::string content;
    std::string filename;
    bool main;
    std::optional<std::array<uint8_t, 32>> signature;

    /// Decodes an embedded script. It is expected to have a header.
    static geode::Result<EmbeddedScript> decode(std::span<const uint8_t>);

    /// Decodes the header of an embedded script. Returns the offset at which zstd compressed data is located.
    static std::optional<size_t> decodeHeader(std::span<const uint8_t>);

    geode::Result<std::vector<uint8_t>> encode(bool includePrefix = true, std::string_view prefixStr = "") const;
};

} // namespace globed