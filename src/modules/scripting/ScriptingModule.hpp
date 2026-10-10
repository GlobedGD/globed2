#pragma once

#include <globed/core/Module.hpp>

namespace globed {

class ScriptingModule : public SoftModule<ScriptingModule> {
public:
    ScriptingModule();

    static constexpr inline auto AUTO_ENABLE = AutoEnableMode::Level;

    static inline const ModuleMetadata metadata {
        .id = "globed.scripting",
        .name = "Scripting",
        .author = "Globed",
    };
};

}
