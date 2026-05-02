#include <stdlib.h>
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <Foundation/Foundation.h>
#include "libanya/log.h"

static NSDictionary *map = nil;

int platform_map_init() {
    NSError *err = nil;

    unsigned long size = 0;
    void *map_plist = getsectiondata((void *)_dyld_get_image_header(0), "__TEXT", "__platform_map", &size);

    if (!map_plist) {
        ANYA_WARNING("no platform map embedded in this build!");
        return -1;
    }

    NSData *data = [NSData dataWithBytesNoCopy:map_plist length:size freeWhenDone:NO];
    map = [NSPropertyListSerialization propertyListWithData:data options:NSPropertyListImmutable format:nil error:&err];

    if (err) {
        ANYA_WARNING("failed to decode embedded platform map?!");
        return -1;
    }

    return 0;
}

#define NSSTRING(cStr) \
    [NSString stringWithUTF8String:cStr]

bool platform_map_soc_supported(const char *soc, const char *rev) {
    NSDictionary *platform = map[@"Platforms"][NSSTRING(soc)];

    if (!platform) {
        return false;
    }

    NSDictionary *revision = platform[@"Revisions"][NSSTRING(rev)];

    if (!revision) {
        return false;
    }

    return true;
}

int platform_map_select_script(const char *soc, const void **script, size_t *script_len) {
    NSDictionary *platform = map[@"Platforms"][NSSTRING(soc)];
    if (!platform) {
        return -1;
    }

    NSString *scriptId = platform[@"Script"];
    NSData *scriptData = map[@"Scripts"][scriptId];

    if (!scriptData) {
        return -1;
    }

    *script = [scriptData bytes];
    *script_len = [scriptData length];

    return 0;
}

int platform_map_select_payload(const char *soc, const char *rev, const void **payload, size_t *payload_len) {
    NSDictionary *platform = map[@"Platforms"][NSSTRING(soc)];
    if (!platform) {
        return -1;
    }

    NSDictionary *revision = platform[@"Revisions"][NSSTRING(rev)];
    if (!revision) {
        return -1;
    }

    NSString *payloadId = revision[@"Payload"];
    NSData *payloadData = map[@"Payloads"][payloadId];

    if (!payloadData) {
        return -1;
    }

    *payload = [payloadData bytes];
    *payload_len = [payloadData length];

    return 0;
}
