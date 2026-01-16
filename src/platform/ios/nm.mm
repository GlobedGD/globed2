#import <Foundation/Foundation.h>

extern "C" {

void* SecTaskCreateFromSelf(CFAllocatorRef allocator);
NSString* SecTaskCopyTeamIdentifier(void* task, NSError** error);
CFTypeRef SecTaskCopyValueForEntitlement(void* task, CFStringRef key, CFErrorRef* error);

}

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

}
