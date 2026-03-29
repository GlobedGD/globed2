#import <Foundation/Foundation.h>
#include <core/preload/PreloadManager.hpp>

extern "C" {

void* SecTaskCreateFromSelf(CFAllocatorRef allocator);
NSString* SecTaskCopyTeamIdentifier(void* task, NSError** error);
CFTypeRef SecTaskCopyValueForEntitlement(void* task, CFStringRef key, CFErrorRef* error);

}

using namespace geode::prelude;

namespace globed {

const char* getIosTeamId() {
    static NSString* ans = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        void* taskSelf = SecTaskCreateFromSelf(NULL);
        CFErrorRef error = NULL;
        CFTypeRef cfans = SecTaskCopyValueForEntitlement(taskSelf, CFSTR("com.apple.developer.team-identifier"), &error);
        if(CFGetTypeID(cfans) == CFStringGetTypeID()) {
            ans = (__bridge NSString*)cfans;
        }
        CFRelease(taskSelf);
        if (!ans)
            ans = [[[NSBundle mainBundle].bundleIdentifier componentsSeparatedByString:@"."] lastObject];
    });
    return ans.UTF8String;
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


}
