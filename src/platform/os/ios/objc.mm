#include <string>

#import <Foundation/Foundation.h>
#import <UIKit/UIDevice.h>

std::string getVendorId() {
    @autoreleasepool {
        NSString *idfv = nil;

        idfv = [[UIDevice currentDevice] identifierForVendor].UUIDString;

        return idfv ? std::string([idfv UTF8String]) : "";
    }
}
