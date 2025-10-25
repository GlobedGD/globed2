#define GEODE_DEFINE_EVENT_EXPORTS

#include <globed/soft-link/API.hpp>
#include <core/net/NetworkManagerImpl.hpp>

using namespace geode::prelude;

namespace globed {

static GlobedApiTable s_functionTable;

Result<FunctionTable*> getFunctionTable() {
    return Ok(&s_functionTable);
}

struct Assigner {
    std::string_view name;

    template <typename T>
    Assigner& operator=(T&& func) {
        s_functionTable.insert(name, std::forward<T>(func));
        return *this;
    }
};

static auto assigner(std::string_view name) {
    return Assigner{ name };
}

#define TABLE_FN(ret, name, ...) \
    assigner(#name) = +[](##__VA_ARGS__) -> Result<ret>

$execute {
    auto& tbl = s_functionTable;

    TABLE_FN(bool, isConnected) {
        return Ok(NetworkManagerImpl::get().isConnected());
    };

    // TODO: impl more stuff

    tbl.finalize();
}

}
