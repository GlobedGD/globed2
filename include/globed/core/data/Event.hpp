#pragma once
#include <asp/ptr/BoxedString.hpp>
#include <span>
#include <vector>
#include <stdint.h>

namespace globed {

struct RawBorrowedEvent {
    asp::BoxedString name;
    int32_t sender;
    std::span<const uint8_t> data;
};

struct RawEvent {
    asp::BoxedString name;
    int32_t sender;
    std::vector<uint8_t> data;
};

}
