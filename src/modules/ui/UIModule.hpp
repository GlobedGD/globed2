#pragma once

#include <globed/core/Module.hpp>

namespace globed {

class UIModule : public SoftModule<UIModule> {
public:
    UIModule();

    static constexpr inline auto AUTO_ENABLE = AutoEnableMode::Server;

    static inline const ModuleMetadata metadata {
        .id = "globed.ui",
        .name = "UI Module",
        .author = "Globed",
    };
};

}

// TODO: small refactor for ui module, prolly move out things that are hookless (the popups/ dir)