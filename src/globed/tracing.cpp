#include "tracing.hpp"

#include <managers/settings.hpp>

bool globed::_traceEnabled() {
    static bool x = GlobedSettings::get().launchArgs().tracing;
    return x;
}
