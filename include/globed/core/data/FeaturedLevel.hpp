#pragma once
#include <stdint.h>

namespace globed {

enum class FeatureTier : uint8_t {
    Normal = 0,
    Epic = 1,
    Outstanding = 2,
};

struct FeaturedLevelMeta {
    int levelId;
    int edition;
    FeatureTier rateTier;
};

} // namespace globed