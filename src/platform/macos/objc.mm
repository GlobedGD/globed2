#include <globed/util/GameState.hpp>
#include <core/preload/PreloadManager.hpp>
#define CommentType CommentTypeDummy
#import <Cocoa/Cocoa.h>
#undef CommentType

using namespace geode::prelude;

namespace globed {

bool isGameFocused() {
    NSRunningApplication* frontApp = [[NSWorkspace sharedWorkspace] frontmostApplication];
    NSString* myBundleId = [[NSBundle mainBundle] bundleIdentifier];
    return [[frontApp bundleIdentifier] isEqualToString:myBundleId];
}

// CCFileUtils::isFileExist on MacOS will always return false if the path is a
// relative path that does not contain any slashes, so we reimplement it properly.
// Thanks cocos!
bool isFileExistImpl(geode::ZStringView path) {
    if (path.empty()) return false;

    auto view = path.view();

    if (view[0] == '/') {
        // absolute path
        return [[NSFileManager defaultManager] fileExistsAtPath:[NSString stringWithUTF8String:path.c_str()]];
    }

    StringBuffer<512> pathBuf;
    StringBuffer<512> fileBuf;
    size_t pos = view.find_last_of('/');
    if (pos != std::string::npos) {
        fileBuf.append(view.substr(pos + 1));
        pathBuf.append(view.substr(0, pos));
    } else {
        fileBuf.append(view);
    }

    NSString* nspath = pathBuf.size() == 0 ? nil : [NSString stringWithUTF8String:pathBuf.c_str()];
    NSString* nsfile = [NSString stringWithUTF8String:fileBuf.c_str()];
    NSString* fullpath = [[NSBundle mainBundle] pathForResource:nsfile ofType:nil inDirectory:nspath];

    return fullpath != nil;
}

std::string getPathForDirAndFilenameImpl(geode::ZStringView directory, geode::ZStringView filename) {
    if (!directory.empty() && directory.view().front() == '/') {
        // absolute path
        geode::utils::StringBuffer<1024> fullBuf;
        fullBuf.append("{}{}", directory, filename);
        if ([[NSFileManager defaultManager] fileExistsAtPath:[NSString stringWithUTF8String:fullBuf.c_str()]]) {
            return fullBuf.str();
        }
    } else {
        // relative path
        NSString* fp = [[NSBundle mainBundle]
            pathForResource:[NSString stringWithUTF8String:filename.c_str()]
            ofType:nil
            inDirectory:[NSString stringWithUTF8String:directory.c_str()]
        ];
        if (fp) {
            return std::string([fp UTF8String]);
        }
    }
    return std::string{};
}

std::unique_ptr<unsigned char[]> getFileDataImpl(geode::ZStringView path, unsigned long* outSize) {
    NSString* nsp = [NSString stringWithUTF8String:path.c_str()];
    NSError* error = nil;
    NSData* data = [NSData dataWithContentsOfFile:nsp options:NSDataReadingMappedIfSafe error:&error];

    if (data) {
        if (outSize) *outSize = [data length];
        auto buffer = std::make_unique<unsigned char[]>([data length]);
        [data getBytes:buffer.get() length:[data length]];
        return buffer;
    }

    if (outSize) *outSize = 0;
    log::warn("Failed to read path '{}': {}", path, [[error localizedDescription] UTF8String]);
    return nullptr;
}

}
