#pragma once

#include <core/hooks/GJBaseGameLayer.hpp>

namespace globed {

inline bool isCustomItem(int itemId)
{
    return itemId >= 80000 && itemId < 100000;
}

inline bool isReadonlyCustomItem(int itemId)
{
    return itemId >= 80000 && itemId < 90000;
}

inline bool isWritableCustomItem(int itemId)
{
    return itemId >= 90000 && itemId < 100000;
}

} // namespace globed
