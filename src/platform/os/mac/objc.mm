#include <string>

#import <Foundation/Foundation.h>
#import <UIKit/UIDevice.h>
#import <AppKit/NSWorkspace.h>

std::string getVendorId() {
    @autoreleasepool {
        NSString *idfv = nil;

        NSURL* bundleURL = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier: [[NSBundle mainBundle] bundleIdentifier]];
        idfv = [[NSFileManager defaultManager] attributesOfItemAtPath:bundleURL.path error:nil][NSFileSystemFileNumber];

        return idfv ? std::string([idfv UTF8String]) : "";
    }
}
