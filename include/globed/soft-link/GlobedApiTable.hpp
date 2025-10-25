#pragma once

#include "FunctionTable.hpp"

#if __has_include(<std23/move_only_function.h>)
# include <std23/move_only_function.h>
#else
# define GLOBED_NO_FUNCTION_ARG
#endif

#define VA_NARGS_IMPL(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define VA_NARGS(...) VA_NARGS_IMPL(_, ## __VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define API_TABLE_FN_0(ret, name) \
    inline ::geode::Result<ret> name() { return this->invoke<ret>(#name); }
#define API_TABLE_FN_1(ret, name, p1) \
    inline ::geode::Result<ret> name(p1 a1) { return this->invoke<ret>(#name, a1); }
#define API_TABLE_FN_2(ret, name, p1, p2) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2) { return this->invoke<ret>(#name, a1, a2); }
#define API_TABLE_FN_3(ret, name, p1, p2, p3) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3) { return this->invoke<ret>(#name, a1, a2, a3); }
#define API_TABLE_FN_4(ret, name, p1, p2, p3, p4) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4) { return this->invoke<ret>(#name, a1, a2, a3, a4); }
#define API_TABLE_FN_5(ret, name, p1, p2, p3, p4, p5) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5) { return this->invoke<ret>(#name, a1, a2, a3, a4, a5); }
#define API_TABLE_FN_6(ret, name, p1, p2, p3, p4, p5, p6) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5, p6 a6) { return this->invoke<ret>(#name, a1, a2, a3, a4, a5, a6); }
#define API_TABLE_FN_7(ret, name, p1, p2, p3, p4, p5, p6, p7) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5, p6 a6, p7 a7) { return this->invoke<ret>(#name, a1, a2, a3, a4, a5, a6, a7); }
#define API_TABLE_FN_8(ret, name, p1, p2, p3, p4, p5, p6, p7, p8) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5, p6 a6, p7 a7, p8 a8) { return this->invoke<ret>(#name, a1, a2, a3, a4, a5, a6, a7, a8); }
#define API_TABLE_FN_9(ret, name, p1, p2, p3, p4, p5, p6, p7, p8, p9) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5, p6 a6, p7 a7, p8 a8, p9 a9) { return this->invoke<ret>(#name, a1, a2, a3, a4, a5, a6, a7, a8, a9); }
#define API_TABLE_FN_10(ret, name, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10) \
    inline ::geode::Result<ret> name(p1 a1, p2 a2, p3 a3, p4 a4, p5 a5, p6 a6, p7 a7, p8 a8, p9 a9, p10 a10) { return this->invoke<ret>(#name, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }
#define API_TABLE_FN(ret, name, ...) GEODE_INVOKE(GEODE_CONCAT(API_TABLE_FN_, VA_NARGS(__VA_ARGS__)), ret, name, __VA_ARGS__)

namespace globed {

struct GlobedApiTable : public FunctionTable {
    API_TABLE_FN(bool, isConnected);
};

}

#undef VA_NARGS_IMPL
#undef VA_NARGS
#undef API_TABLE_FN_0
#undef API_TABLE_FN_1
#undef API_TABLE_FN_2
#undef API_TABLE_FN_3
#undef API_TABLE_FN_4
#undef API_TABLE_FN_5
#undef API_TABLE_FN_6
#undef API_TABLE_FN_7
#undef API_TABLE_FN_8
#undef API_TABLE_FN_9
#undef API_TABLE_FN_10
