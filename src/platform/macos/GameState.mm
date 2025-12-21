#include <globed/util/GameState.hpp>
#import <Cocoa/Cocoa.h>

using namespace geode::prelude;

namespace globed {

bool isGameFocused() {
    NSRunningApplication* frontApp = [[NSWorkspace sharedWorkspace] frontmostApplication];
    NSString* myBundleId = [[NSBundle mainBundle] bundleIdentifier];
    return [[frontApp bundleIdentifier] isEqualToString:myBundleId];
}

}
