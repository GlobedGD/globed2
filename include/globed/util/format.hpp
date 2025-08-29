#pragma once

#include <Geode/Result.hpp>
#include <vector>
#include <stddef.h>
#include <stdint.h>

namespace globed {

std::string hexEncode(const void* data, size_t size);
geode::Result<std::vector<uint8_t>> hexDecode(std::string_view data);

}