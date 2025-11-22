//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#define __OBJC__ 1
#include "../../platform.h"
#include "macosx_metal.h"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Cocoa/Cocoa.h>

static MetalContext g_metal_context = {};

MetalContext* GetMetalContext()
{
    return &g_metal_context;
}

void InitMetal(const RendererTraits* traits, NSWindow* window, CAMetalLayer* layer)
{
    (void)traits;
    (void)window;

    @autoreleasepool {
        // Get default Metal device
        g_metal_context.device = MTLCreateSystemDefaultDevice();
        if (!g_metal_context.device) {
            NSLog(@"Metal is not supported on this device");
            return;
        }

        // Store the layer
        g_metal_context.metal_layer = layer;
        layer.device = g_metal_context.device;
        layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        layer.framebufferOnly = YES;

        // Create command queue
        g_metal_context.command_queue = [g_metal_context.device newCommandQueue];

        // Create semaphore for frame synchronization
        g_metal_context.frame_semaphore = dispatch_semaphore_create(3);

        // Try to load default Metal library
        NSError* error = nil;
        g_metal_context.default_library = [g_metal_context.device newDefaultLibrary];
        if (!g_metal_context.default_library) {
            NSLog(@"Failed to load default Metal library: %@", error);
        }

        g_metal_context.is_rendering = false;

        NSLog(@"Metal initialized successfully");
        NSLog(@"Device: %@", [g_metal_context.device name]);
    }
}

void ResizeMetal(const Vec2Int& screen_size)
{
    @autoreleasepool {
        if (g_metal_context.metal_layer) {
            g_metal_context.metal_layer.drawableSize = CGSizeMake(screen_size.x, screen_size.y);
        }
    }
}

void ShutdownMetal()
{
    @autoreleasepool {
        // Wait for all frames to complete
        if (g_metal_context.frame_semaphore) {
            for (int i = 0; i < 3; i++) {
                dispatch_semaphore_wait(g_metal_context.frame_semaphore, DISPATCH_TIME_FOREVER);
            }
        }

        // Clean up resources
        g_metal_context.render_encoder = nil;
        g_metal_context.command_buffer = nil;
        g_metal_context.current_drawable = nil;
        g_metal_context.default_library = nil;
        g_metal_context.command_queue = nil;
        g_metal_context.device = nil;
        g_metal_context.metal_layer = nil;

        if (g_metal_context.frame_semaphore) {
            // Release the semaphore (ARC doesn't handle dispatch objects in older systems)
            #if !__has_feature(objc_arc)
            dispatch_release(g_metal_context.frame_semaphore);
            #endif
            g_metal_context.frame_semaphore = nil;
        }

        g_metal_context = {};
    }
}

void WaitMetal()
{
    @autoreleasepool {
        // Wait for all pending command buffers to complete
        if (g_metal_context.command_queue) {
            id<MTLCommandBuffer> waitBuffer = [g_metal_context.command_queue commandBuffer];
            [waitBuffer commit];
            [waitBuffer waitUntilCompleted];
        }
    }
}
