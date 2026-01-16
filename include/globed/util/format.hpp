#pragma once

#include <globed/config.hpp>

#include <Geode/Result.hpp>
#include <span>
#include <stddef.h>
#include <stdint.h>
#include <vector>

namespace globed {

GLOBED_DLL std::string hexEncode(const void *data, size_t size);
GLOBED_DLL std::string hexEncode(std::span<const uint8_t> data);
GLOBED_DLL std::string hexEncode(const std::vector<uint8_t> &data);
GLOBED_DLL geode::Result<std::vector<uint8_t>> hexDecode(std::string_view data);

} // namespace globed