#pragma once

#include "GlobedApiTable.hpp"
#include <Geode/loader/Dispatch.hpp>

#undef MY_MOD_ID
#define MY_MOD_ID "dankmeme.globed2"

namespace globed {

static FunctionTable dummyTable;

inline geode::Result<FunctionTable*> getFunctionTable() GEODE_EVENT_EXPORT(&getFunctionTable, ());

inline auto api() {
    static FunctionTable* table = nullptr;
    if (!table) {
        if (auto res = getFunctionTable()) {
            table = *res;
            if (table->abi() != GLOBED_ABI) {
                geode::log::warn("Globed ABI mismatch: expecting {}, but globed uses {}", GLOBED_ABI, table->abi());
                table = nullptr;
            }
        } else {
            geode::log::warn("Failed to get Globed function table: {}", res.unwrapErr());
        }
    }

    return static_cast<GlobedApiTable*>(table ? table : &dummyTable);
}

}