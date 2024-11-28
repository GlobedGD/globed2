#pragma once

#include <defs/assert.hpp>

namespace globed {
    constexpr inline int CUSTOM_ITEM_ID_RO_START = 80'000;
    constexpr inline int CUSTOM_ITEM_ID_W_START = 90'000;
    constexpr inline int CUSTOM_ITEM_ID_END = 100'000;

#define $rid(purpose, x) constexpr inline int ITEM_##purpose = CUSTOM_ITEM_ID_RO_START + x

    $rid(ACCOUNT_ID, 0);
    $rid(LAST_JOINED, 1);
    $rid(LAST_LEFT, 2);
    $rid(TOTAL_PLAYERS, 3);
    $rid(TOTAL_PLAYERS_JOINED, 4);
    $rid(TOTAL_PLAYERS_LEFT, 5);
    $rid(POSITION_X, 50);
    $rid(POSITION_Y, 51);

#undef $rid

    inline bool isCustomItem(int itemId) {
        return itemId >= CUSTOM_ITEM_ID_RO_START && itemId < CUSTOM_ITEM_ID_END;
    }

    inline bool isWritableCustomItem(int itemId) {
        return itemId >= CUSTOM_ITEM_ID_W_START && itemId < CUSTOM_ITEM_ID_END;
    }

    inline bool isReadonlyCustomItem(int itemId) {
        return itemId >= CUSTOM_ITEM_ID_RO_START && itemId < CUSTOM_ITEM_ID_W_START;
    }

    // big value -> small value
    inline uint16_t itemIdToCustom(int itemId) {
#ifdef GLOBED_DEBUG
        GLOBED_REQUIRE(isCustomItem(itemId), "not custom item ID passed to itemIdToCustom");
#endif

        return itemId - CUSTOM_ITEM_ID_W_START;
    }

    inline int customItemToItemId(uint16_t customItemId) {
#ifdef GLOBED_DEBUG
        GLOBED_REQUIRE(customItemId >= 0 && customItemId < (CUSTOM_ITEM_ID_END - CUSTOM_ITEM_ID_W_START), "invalid custom item ID passed to customItemToItemId");
#endif

        return CUSTOM_ITEM_ID_W_START + customItemId;
    }
}