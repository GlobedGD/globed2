#pragma once

#include <Geode/Geode.hpp>

namespace globed {

// vanilla object ids
struct ObjectId {
    typedef enum {
        ItemEditTrigger = 3619,
        ItemCompareTrigger = 3620,
    } Id;

    Id id;

    inline ObjectId(Id id) : id(id) {}

    operator int() const {
        return (int)id;
    }
};

// custom object ids
constexpr inline uint32_t SCRIPT_OBJECT_IDENT_MASK = 0x71eb0000; // top bit must be 0 :)
constexpr inline uint32_t SCRIPT_OBJECT_TYPE_MASK = 0x0000ffff;

enum class ScriptObjectType : uint32_t {
    None = 0x0,
    FireServer = 0x1,
    ListenEvent = 0x2,
};

}
