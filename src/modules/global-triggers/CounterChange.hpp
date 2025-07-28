#pragma once
#include <stdint.h>

namespace globed {

enum class CounterChangeType {
    Set = 0,
    Add = 1,
    Multiply = 2,
    Divide = 3,

    Last = Divide,
};

union CounterChangeValue {
    int32_t intValue;
    float floatValue;

    inline int32_t& asInt() {
        return intValue;
    }

    inline int32_t asInt() const {
        return intValue;
    }

    inline float& asFloat() {
        return floatValue;
    }

    inline float asFloat() const {
        return floatValue;
    }
};

struct CounterChange {
    uint32_t itemId;
    CounterChangeValue value;
    CounterChangeType type;

    CounterChange(uint32_t itemId, int32_t value, CounterChangeType type) : itemId(itemId), type(type) {
        this->value.intValue = value;
    }

    CounterChange(uint32_t itemId, float value, CounterChangeType type) : itemId(itemId), type(type) {
        this->value.floatValue = value;
    }

    CounterChange() : CounterChange(0, 0, CounterChangeType::Set) {}
};

}
