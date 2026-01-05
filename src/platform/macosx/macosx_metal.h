//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#else
// Forward declarations for C++ code
typedef struct objc_object CAMetalLayer;
typedef struct objc_object MTLDevice;
typedef struct objc_object MTLCommandQueue;
typedef struct objc_object MTLRenderPipelineState;
typedef struct objc_object MTLLibrary;
typedef struct objc_object MTLFunction;
typedef struct objc_object MTLBuffer;
typedef struct objc_object MTLTexture;
typedef struct objc_object MTLSamplerState;
typedef struct objc_object MTLCommandBuffer;
typedef struct objc_object MTLRenderCommandEncoder;
typedef struct objc_object CAMetalDrawable;
#endif

// Metal context - holds global Metal state
struct MetalContext {
    CAMetalLayer* metal_layer;
    id<MTLDevice> device;
    id<MTLCommandQueue> command_queue;
    id<MTLLibrary> default_library;

    // Current rendering state
    id<MTLCommandBuffer> command_buffer;
    id<MTLRenderCommandEncoder> render_encoder;
    id<CAMetalDrawable> current_drawable;

    // Synchronization
    dispatch_semaphore_t frame_semaphore;

    bool is_rendering;
};

// Function prototypes
extern MetalContext* GetMetalContext();
