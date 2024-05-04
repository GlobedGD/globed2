#include "util.hpp"

#include <managers/resource.hpp>

const std::string& operator ""_gstr(const char* key, size_t length) {
    return GlobedResourceManager::get().getString(std::string_view(key, length));
}