#pragma once

#include <Geode/Bindings.hpp>

namespace util::gd {
    void reorderDownloadedLevel(GJGameLevel* level);
    void openProfile(int accountId, int userId, const std::string& name);
    int calcLevelDifficulty(GJGameLevel* level);
}