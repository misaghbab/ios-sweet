/* Diagnostic gate for an experimental Metal backend; does not install a driver. */
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <CoreFoundation/CoreFoundation.h>
/* iPhoneOS SDK ships the framework stubs but omits the IOSurface C headers. */
typedef struct __IOSurface *IOSurfaceRef;
extern IOSurfaceRef IOSurfaceCreate(CFDictionaryRef properties);
extern const CFStringRef kIOSurfaceWidth, kIOSurfaceHeight;
extern const CFStringRef kIOSurfaceBytesPerElement, kIOSurfaceBytesPerRow;
extern const CFStringRef kIOSurfaceAllocSize, kIOSurfacePixelFormat;
#include <dispatch/dispatch.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *stage)
{
    fprintf(stderr, "ALIOTH METAL: FAIL %s\n", stage);
    return 1;
}

int main(void)
{
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) return fail("device unavailable");
        printf("ALIOTH METAL: device=%s\n", device.name.UTF8String);
        id<MTLCommandQueue> queue = [device newCommandQueue];
        if (!queue) return fail("newCommandQueue returned nil");

        id<MTLBuffer> buffer = [device newBufferWithLength:4096
                                                options:MTLResourceStorageModeShared];
        if (!buffer || !buffer.contents) return fail("shared buffer");
        memset(buffer.contents, 0, 4096);
        id<MTLCommandBuffer> command = [queue commandBuffer];
        if (!command) return fail("command buffer");
        id<MTLBlitCommandEncoder> blit = [command blitCommandEncoder];
        if (!blit) return fail("blit encoder");
        [blit fillBuffer:buffer range:NSMakeRange(0, 4096) value:0x5a];
        [blit endEncoding];
        dispatch_semaphore_t done = dispatch_semaphore_create(0);
        [command addCompletedHandler:^(id<MTLCommandBuffer> finished) {
            (void)finished;
            dispatch_semaphore_signal(done);
        }];
        [command commit];
        if (dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW, 10 * NSEC_PER_SEC)))
            return fail("completion timeout");
        if (command.status != MTLCommandBufferStatusCompleted) {
            fprintf(stderr, "ALIOTH METAL: %s\n", command.error.description.UTF8String ?: "no error detail");
            return fail("command execution");
        }
        const unsigned char *bytes = buffer.contents;
        for (size_t i = 0; i < 4096; ++i)
            if (bytes[i] != 0x5a) return fail("buffer readback mismatch");

        NSDictionary *properties = @{
            (NSString *)kIOSurfaceWidth: @16, (NSString *)kIOSurfaceHeight: @16,
            (NSString *)kIOSurfaceBytesPerElement: @4,
            (NSString *)kIOSurfaceBytesPerRow: @64,
            (NSString *)kIOSurfaceAllocSize: @1024,
            (NSString *)kIOSurfacePixelFormat: @(0x42475241)
        };
        IOSurfaceRef surface = IOSurfaceCreate((__bridge CFDictionaryRef)properties);
        if (!surface) return fail("IOSurface creation");
        MTLTextureDescriptor *descriptor = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                        width:16 height:16 mipmapped:NO];
        descriptor.storageMode = MTLStorageModeShared;
        descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor iosurface:surface plane:0];
        CFRelease(surface);
        if (!texture) return fail("IOSurface texture import");
        puts("ALIOTH METAL: PASS queue, blit readback, IOSurface texture creation");
        puts("ALIOTH METAL: shader execution and wallpaper rendering still require separate tests");
        return 0;
    }
}
