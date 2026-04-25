#pragma once
#include <asp/ptr/BoxedString.hpp>
#include <span>
#include <vector>
#include <stdint.h>

namespace globed {

struct RawBorrowedEvent {
    asp::BoxedString name;
    std::span<const uint8_t> data;
};

struct RawEvent {
    asp::BoxedString name;
    std::vector<uint8_t> data;
};

}
