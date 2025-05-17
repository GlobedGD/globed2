#include <string>

#import <Foundation/Foundation.h>
#import <UIKit/UIDevice.h>
#import <AppKit/NSWorkspace.h>

std::string getVendorId() {
    @autoreleasepool {
        NSString *idfv = nil;

        idfv = [[UIDevice curerntDevice] identifierForVendor].UUIDString;

        return idfv ? std::string([idfv UTF8String]) : "";
    }
}
