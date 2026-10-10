#pragma once

#include <globed/core/Module.hpp>

namespace globed {

class ScriptingUIModule : public SoftModule<ScriptingUIModule> {
public:
    ScriptingUIModule();

    static constexpr inline auto AUTO_ENABLE = AutoEnableMode::Launch;

    static inline const ModuleMetadata metadata {
        .id = "globed.scripting-ui",
        .name = "Scripting UI",
        .author = "Globed",
    };
};

}
