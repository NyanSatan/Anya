#import <Foundation/Foundation.h>
#import "libanya/utils.h"
#import "libanya/log.h"

#define KBAG_LEN_256    (48)

int batch_init(const char *input, void **ctx) {
    NSError *err = nil;
    NSData *d = [NSData dataWithContentsOfFile:[NSString stringWithCString:input encoding:NSUTF8StringEncoding]
                                       options:0
                                         error:&err];
    if (err) {
        ANYA_ERROR("failed to read JSON: %s", [[err description] UTF8String]);
        return -1;
    }

    NSArray *kbags = [NSJSONSerialization JSONObjectWithData:d
                                                     options:NSJSONReadingMutableContainers
                                                       error:&err];

    if (err) {
        ANYA_ERROR("failed to parse JSON: %s", [[err description] UTF8String]);
        return -1;
    }

    *ctx = (void *)CFBridgingRetain(kbags);

    return 0;
}

int batch_get_count(void *ctx) {
    return [(__bridge NSArray *)ctx count];
}

int batch_get_kbag(void *ctx, int idx, uint8_t *kbag) {
    NSString *kbagObj = [(__bridge NSArray *)ctx objectAtIndex:idx][@"kbag"];
    if (!kbagObj) {
        return -1;
    }

    NSUInteger kbagLen = [kbagObj length] / 2;

    if (kbagLen > KBAG_LEN_256) {
        ANYA_ERROR("KBAG @ index %d is too long", idx);
        return -1;
    }

    str2hex([kbagObj length], kbag, [kbagObj UTF8String]);

    return 0;
}

int batch_set_key(void *ctx, int idx, uint8_t *kbag) {
    NSMutableDictionary *entry = [(__bridge NSArray *)ctx objectAtIndex:idx];

    char tmp[KBAG_LEN_256 * 2 + 1];
    hex2str(tmp, KBAG_LEN_256, kbag);

    entry[@"key"] = [NSString stringWithUTF8String:tmp];

    return 0;
}

int batch_write(void *ctx, const char *path) {
    NSError *err = nil;

    NSJSONWritingOptions opts = NSJSONWritingPrettyPrinted;

    if (@available(macOS 10.15, *)) {
        opts |= NSJSONWritingWithoutEscapingSlashes;
    }

    NSData *d = [NSJSONSerialization dataWithJSONObject:(__bridge id)ctx options:opts error:&err];
    if (err) {
        ANYA_ERROR("failed to serialize JSON: %s\n", [[err description] UTF8String]);
        return -1;
    }

    if (![d writeToFile:[NSString stringWithUTF8String:path] atomically:NO]) {
        ANYA_ERROR("failed to write the batch");
        return -1;
    }

    return 0;
}

void batch_quiesce(void **ctx) {
    *ctx = NULL;
}
