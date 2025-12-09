//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#define __OBJC__ 1
#include "../../platform.h"
#include "macosx_metal.h"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <simd/simd.h>

enum UniformBufferType
{
    UNIFORM_BUFFER_CAMERA,
    UNIFORM_BUFFER_OBJECT,
    UNIFORM_BUFFER_VERTEX_USER,
    UNIFORM_BUFFER_COLOR,
    UNIFORM_BUFFER_LIGHT,
    UNIFORM_BUFFER_FRAGMENT_USER,
    UNIFORM_BUFFER_COUNT
};

constexpr int MAX_TEXTURES = 128;
constexpr int MAX_UNIFORM_BUFFERS = 8192;
constexpr u32 DYNAMIC_UNIFORM_BUFFER_SIZE = MAX_UNIFORM_BUFFER_SIZE * MAX_UNIFORM_BUFFERS;

const char* UNIFORM_BUFFER_NAMES[] = {
    "CameraBuffer",
    "TransformBuffer",
    "VertexUserBuffer",
    "ColorBuffer",
    "LightBuffer",
    "FragmentUserBuffer"
};
static_assert(sizeof(UNIFORM_BUFFER_NAMES) / sizeof(const char*) == UNIFORM_BUFFER_COUNT);

static MTLSamplerMinMagFilter ToMetal(TextureFilter filter) {
    return filter == TEXTURE_FILTER_LINEAR ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
}

static MTLSamplerAddressMode ToMetal(TextureClamp clamp) {
    return clamp == TEXTURE_CLAMP_REPEAT ? MTLSamplerAddressModeRepeat : MTLSamplerAddressModeClampToEdge;
}

static void CopyMat3ToGPU(void* dst, const Mat3& src) {
    float *f = (float*)dst;
    f[0] = src.m[0]; f[1] = src.m[1]; f[2] = src.m[2]; f[3] = 0.0f;
    f[4] = src.m[3]; f[5] = src.m[4]; f[6] = src.m[5]; f[7] = 0.0f;
    f[8] = src.m[6]; f[9] = src.m[7]; f[10]= src.m[8]; f[11]= 0.0f;
}

struct platform::Texture
{
    id<MTLTexture> metal_texture;
    id<MTLSamplerState> metal_sampler;
    SamplerOptions sampler_options;
    Vec2Int size;
    i32 channels;
};

struct platform::Shader
{
    id<MTLRenderPipelineState> pipeline_state;
    id<MTLFunction> vertex_function;
    id<MTLFunction> fragment_function;
    ShaderFlags flags;
};

struct platform::Buffer
{
    id<MTLBuffer> metal_buffer;
    u32 size;
};

struct ObjectBuffer {
    float transform[12];
    float depth;
    float depth_scale;
};

struct ColorBuffer {
    Color color;
    Color emission;
    Vec2 uv_offset;
    Vec2 padding;
};

struct MetalDynamicBuffer
{
    id<MTLBuffer> buffer;
    u32 offset;
    u32 size;
};

struct MetalRenderer {
    RendererTraits traits;
    MetalContext* context;

    // Dynamic uniform buffers
    MetalDynamicBuffer uniform_buffers[UNIFORM_BUFFER_COUNT];
    u32 uniform_buffer_offsets[UNIFORM_BUFFER_COUNT];
    u32 frame_index;

    // Current render state
    platform::Shader* current_shader;
    platform::Texture* bound_textures[MAX_TEXTURES];
    Color clear_color;

    // Vertex buffers
    id<MTLBuffer> current_vertex_buffer;
    id<MTLBuffer> current_index_buffer;
};

static MetalRenderer g_renderer = {};

void platform::BeginRenderFrame()
{
    @autoreleasepool {
        MetalContext* ctx = GetMetalContext();

        // Wait for available frame
        dispatch_semaphore_wait(ctx->frame_semaphore, DISPATCH_TIME_FOREVER);

        // Get next drawable
        ctx->current_drawable = [ctx->metal_layer nextDrawable];
        if (!ctx->current_drawable) {
            dispatch_semaphore_signal(ctx->frame_semaphore);
            return;
        }

        // Create command buffer
        ctx->command_buffer = [ctx->command_queue commandBuffer];

        // Add completion handler to signal semaphore
        [ctx->command_buffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
            (void)buffer;
            dispatch_semaphore_signal(ctx->frame_semaphore);
        }];

        // Reset uniform buffer offsets
        for (int i = 0; i < UNIFORM_BUFFER_COUNT; i++) {
            g_renderer.uniform_buffer_offsets[i] = 0;
        }

        g_renderer.frame_index++;
    }
}

void platform::EndRenderFrame()
{
    @autoreleasepool {
        MetalContext* ctx = GetMetalContext();

        if (ctx->current_drawable && ctx->command_buffer) {
            [ctx->command_buffer presentDrawable:ctx->current_drawable];
            [ctx->command_buffer commit];
        }

        ctx->current_drawable = nil;
        ctx->command_buffer = nil;
    }
}

void platform::BeginRenderPass(Color clear_color)
{
    @autoreleasepool {
        MetalContext* ctx = GetMetalContext();
        g_renderer.clear_color = clear_color;

        if (!ctx->current_drawable || !ctx->command_buffer)
            return;

        // Create render pass descriptor
        MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        renderPassDescriptor.colorAttachments[0].texture = ctx->current_drawable.texture;
        renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(
            clear_color.r,
            clear_color.g,
            clear_color.b,
            clear_color.a
        );
        renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

        // Create render command encoder
        ctx->render_encoder = [ctx->command_buffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        ctx->is_rendering = true;
    }
}

void platform::EndRenderPass()
{
    @autoreleasepool {
        MetalContext* ctx = GetMetalContext();

        if (ctx->render_encoder) {
            [ctx->render_encoder endEncoding];
            ctx->render_encoder = nil;
        }

        ctx->is_rendering = false;
    }
}

void platform::SetViewport(const noz::Rect& viewport)
{
    @autoreleasepool {
        MetalContext* ctx = GetMetalContext();
        if (!ctx->render_encoder)
            return;

        MTLViewport mtl_viewport;
        MTLScissorRect scissor;

        if (viewport.width > 0 && viewport.height > 0) {
            // Use specified viewport
            mtl_viewport = {
                .originX = (double)viewport.x,
                .originY = (double)viewport.y,
                .width = (double)viewport.width,
                .height = (double)viewport.height,
                .znear = 0.0,
                .zfar = 1.0
            };
            scissor = {
                .x = (NSUInteger)viewport.x,
                .y = (NSUInteger)viewport.y,
                .width = (NSUInteger)viewport.width,
                .height = (NSUInteger)viewport.height
            };
        } else {
            // Reset to full screen
            CGSize size = ctx->metal_layer.drawableSize;
            mtl_viewport = {
                .originX = 0.0,
                .originY = 0.0,
                .width = size.width,
                .height = size.height,
                .znear = 0.0,
                .zfar = 1.0
            };
            scissor = {
                .x = 0,
                .y = 0,
                .width = (NSUInteger)size.width,
                .height = (NSUInteger)size.height
            };
        }

        [ctx->render_encoder setViewport:mtl_viewport];
        [ctx->render_encoder setScissorRect:scissor];
    }
}

void platform::BindTransform(const Mat3& transform, float depth, float depth_scale)
{
    @autoreleasepool {
        ObjectBuffer obj_buffer;

        // Copy transform matrix (3x3 stored as 3x4)
        CopyMat3ToGPU(&obj_buffer.transform, transform);

        obj_buffer.depth = depth;
        obj_buffer.depth_scale = depth_scale;

        // Update uniform buffer
        MetalDynamicBuffer& uniform = g_renderer.uniform_buffers[UNIFORM_BUFFER_OBJECT];
        u32& offset = g_renderer.uniform_buffer_offsets[UNIFORM_BUFFER_OBJECT];

        if (offset + sizeof(ObjectBuffer) <= uniform.size) {
            memcpy((u8*)[uniform.buffer contents] + offset, &obj_buffer, sizeof(ObjectBuffer));

            MetalContext* ctx = GetMetalContext();
            if (ctx->render_encoder) {
                [ctx->render_encoder setVertexBuffer:uniform.buffer
                                               offset:offset
                                              atIndex:UNIFORM_BUFFER_OBJECT];
            }

            offset += MAX_UNIFORM_BUFFER_SIZE;
        }
    }
}

void platform::BindVertexUserData(const u8* data, u32 size)
{
    @autoreleasepool {
        MetalDynamicBuffer& uniform = g_renderer.uniform_buffers[UNIFORM_BUFFER_VERTEX_USER];
        u32& offset = g_renderer.uniform_buffer_offsets[UNIFORM_BUFFER_VERTEX_USER];

        if (offset + size <= uniform.size) {
            memcpy((u8*)[uniform.buffer contents] + offset, data, size);

            MetalContext* ctx = GetMetalContext();
            if (ctx->render_encoder) {
                [ctx->render_encoder setVertexBuffer:uniform.buffer
                                               offset:offset
                                              atIndex:UNIFORM_BUFFER_VERTEX_USER];
            }

            offset += MAX_UNIFORM_BUFFER_SIZE;
        }
    }
}

void platform::BindFragmentUserData(const u8* data, u32 size)
{
    @autoreleasepool {
        MetalDynamicBuffer& uniform = g_renderer.uniform_buffers[UNIFORM_BUFFER_FRAGMENT_USER];
        u32& offset = g_renderer.uniform_buffer_offsets[UNIFORM_BUFFER_FRAGMENT_USER];

        if (offset + size <= uniform.size) {
            memcpy((u8*)[uniform.buffer contents] + offset, data, size);

            MetalContext* ctx = GetMetalContext();
            if (ctx->render_encoder) {
                [ctx->render_encoder setFragmentBuffer:uniform.buffer
                                                 offset:offset
                                                atIndex:UNIFORM_BUFFER_FRAGMENT_USER];
            }

            offset += MAX_UNIFORM_BUFFER_SIZE;
        }
    }
}

void platform::BindCamera(const Mat3& view_matrix) {
    @autoreleasepool {
        struct CameraBuffer {
            float view[12];
        } camera_buffer;

        // Copy view matrix
        CopyMat3ToGPU(&camera_buffer.view, view_matrix);

        MetalDynamicBuffer& uniform = g_renderer.uniform_buffers[UNIFORM_BUFFER_CAMERA];
        u32& offset = g_renderer.uniform_buffer_offsets[UNIFORM_BUFFER_CAMERA];

        if (offset + sizeof(CameraBuffer) <= uniform.size) {
            memcpy((u8*)[uniform.buffer contents] + offset, &camera_buffer, sizeof(CameraBuffer));

            MetalContext* ctx = GetMetalContext();
            if (ctx->render_encoder) {
                [ctx->render_encoder setVertexBuffer:uniform.buffer
                                               offset:offset
                                              atIndex:UNIFORM_BUFFER_CAMERA];
            }

            offset += MAX_UNIFORM_BUFFER_SIZE;
        }
    }
}

void platform::BindColor(const Color& color, const Vec2& color_uv_offset, const Color& emission)
{
    @autoreleasepool {
        ColorBuffer color_buffer;
        color_buffer.color = color;
        color_buffer.emission = emission;
        color_buffer.uv_offset = color_uv_offset;

        MetalDynamicBuffer& uniform = g_renderer.uniform_buffers[UNIFORM_BUFFER_COLOR];
        u32& offset = g_renderer.uniform_buffer_offsets[UNIFORM_BUFFER_COLOR];

        if (offset + sizeof(ColorBuffer) <= uniform.size) {
            memcpy((u8*)[uniform.buffer contents] + offset, &color_buffer, sizeof(ColorBuffer));

            MetalContext* ctx = GetMetalContext();
            if (ctx->render_encoder) {
                [ctx->render_encoder setFragmentBuffer:uniform.buffer
                                                 offset:offset
                                                atIndex:UNIFORM_BUFFER_COLOR];
            }

            offset += MAX_UNIFORM_BUFFER_SIZE;
        }
    }
}

platform::Buffer* platform::CreateVertexBuffer(
    const MeshVertex* vertices,
    u16 vertex_count,
    const char* name)
{
    @autoreleasepool {
        MetalContext* ctx = GetMetalContext();

        u32 size = vertex_count * sizeof(MeshVertex);
        id<MTLBuffer> metal_buffer = [ctx->device newBufferWithBytes:vertices
                                                              length:size
                                                             options:MTLResourceStorageModeShared];

        if (name) {
            metal_buffer.label = [NSString stringWithUTF8String:name];
        }

        Buffer* buffer = (Buffer*)Alloc(ALLOCATOR_DEFAULT, sizeof(Buffer));
        buffer->metal_buffer = metal_buffer;
        buffer->size = size;

        return buffer;
    }
}

platform::Buffer* platform::CreateIndexBuffer(const u16* indices, u16 index_count, const char* name)
{
    @autoreleasepool {
        MetalContext* ctx = GetMetalContext();

        u32 size = index_count * sizeof(u16);
        id<MTLBuffer> metal_buffer = [ctx->device newBufferWithBytes:indices
                                                              length:size
                                                             options:MTLResourceStorageModeShared];

        if (name) {
            metal_buffer.label = [NSString stringWithUTF8String:name];
        }

        Buffer* buffer = (Buffer*)Alloc(ALLOCATOR_DEFAULT, sizeof(Buffer));
        buffer->metal_buffer = metal_buffer;
        buffer->size = size;

        return buffer;
    }
}

void platform::DestroyBuffer(Buffer* buffer)
{
    if (buffer) {
        buffer->metal_buffer = nil;
        Free(buffer);
    }
}

void platform::BindVertexBuffer(Buffer* buffer)
{
    @autoreleasepool {
        if (buffer) {
            g_renderer.current_vertex_buffer = buffer->metal_buffer;

            MetalContext* ctx = GetMetalContext();
            if (ctx->render_encoder) {
                [ctx->render_encoder setVertexBuffer:buffer->metal_buffer
                                               offset:0
                                              atIndex:0];
            }
        }
    }
}

void platform::BindIndexBuffer(Buffer* buffer)
{
    if (buffer) {
        g_renderer.current_index_buffer = buffer->metal_buffer;
    }
}

void platform::DrawIndexed(u16 index_count)
{
    @autoreleasepool {
        MetalContext* ctx = GetMetalContext();

        if (ctx->render_encoder && g_renderer.current_index_buffer) {
            [ctx->render_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                            indexCount:index_count
                                             indexType:MTLIndexTypeUInt16
                                           indexBuffer:g_renderer.current_index_buffer
                                     indexBufferOffset:0];
        }
    }
}

void platform::BindTexture(Texture* texture, int slot)
{
    @autoreleasepool {
        if (slot >= 0 && slot < MAX_TEXTURES) {
            g_renderer.bound_textures[slot] = texture;

            MetalContext* ctx = GetMetalContext();
            if (ctx->render_encoder && texture) {
                [ctx->render_encoder setFragmentTexture:texture->metal_texture
                                                 atIndex:slot];
                [ctx->render_encoder setFragmentSamplerState:texture->metal_sampler
                                                      atIndex:slot];
            }
        }
    }
}

platform::Texture* platform::CreateTexture(
    void* data,
    size_t width,
    size_t height,
    int channels,
    const SamplerOptions& sampler_options,
    const char* name)
{
    @autoreleasepool {
        MetalContext* ctx = GetMetalContext();

        // Create texture descriptor
        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                              width:width
                                                                                             height:height
                                                                                          mipmapped:NO];
        descriptor.usage = MTLTextureUsageShaderRead;
        descriptor.storageMode = MTLStorageModeShared;

        id<MTLTexture> metal_texture = [ctx->device newTextureWithDescriptor:descriptor];

        if (name) {
            metal_texture.label = [NSString stringWithUTF8String:name];
        }

        // Upload texture data
        if (data) {
            MTLRegion region = MTLRegionMake2D(0, 0, width, height);

            // Convert data to RGBA if needed
            if (channels == 4) {
                [metal_texture replaceRegion:region
                                  mipmapLevel:0
                                    withBytes:data
                                  bytesPerRow:width * 4];
            } else {
                // Need to convert to RGBA
                u8* rgba_data = (u8*)Alloc(ALLOCATOR_SCRATCH, width * height * 4);
                for (size_t i = 0; i < width * height; i++) {
                    u8* src = (u8*)data + i * channels;
                    u8* dst = rgba_data + i * 4;

                    if (channels == 1) {
                        dst[0] = dst[1] = dst[2] = src[0];
                        dst[3] = 255;
                    } else if (channels == 3) {
                        dst[0] = src[0];
                        dst[1] = src[1];
                        dst[2] = src[2];
                        dst[3] = 255;
                    }
                }

                [metal_texture replaceRegion:region
                                  mipmapLevel:0
                                    withBytes:rgba_data
                                  bytesPerRow:width * 4];

                Free(rgba_data);
            }
        }

        // Create sampler
        MTLSamplerDescriptor* samplerDesc = [MTLSamplerDescriptor new];
        samplerDesc.minFilter = ToMetal(sampler_options.filter);
        samplerDesc.magFilter = ToMetal(sampler_options.filter);
        samplerDesc.sAddressMode = ToMetal(sampler_options.clamp);
        samplerDesc.tAddressMode = ToMetal(sampler_options.clamp);

        id<MTLSamplerState> metal_sampler = [ctx->device newSamplerStateWithDescriptor:samplerDesc];

        // Create platform texture
        Texture* texture = (Texture*)Alloc(ALLOCATOR_DEFAULT, sizeof(Texture));
        texture->metal_texture = metal_texture;
        texture->metal_sampler = metal_sampler;
        texture->sampler_options = sampler_options;
        texture->size = Vec2Int{(i32)width, (i32)height};
        texture->channels = channels;

        return texture;
    }
}

void platform::DestroyTexture(Texture* texture)
{
    if (texture) {
        texture->metal_texture = nil;
        texture->metal_sampler = nil;
        Free(texture);
    }
}

platform::Shader* platform::CreateShader(
    const void* vertex_spirv,
    u32 vertex_spirv_size,
    const void* geometry_spirv,
    u32 geometry_spirv_size,
    const void* fragment_spirv,
    u32 fragment_spirv_size,
    const char* vertex_glsl,
    u32 vertex_glsl_size,
    const char* geometry_glsl,
    u32 geometry_glsl_size,
    const char* fragment_glsl,
    u32 fragment_glsl_size,
    ShaderFlags flags,
    const char* name)
{
    @autoreleasepool {
        // Metal uses its own shader language - currently expects Metal source in vertex_spirv/fragment_spirv
        // TODO: Add Metal shader support to the importer
        (void)geometry_spirv;
        (void)geometry_spirv_size;
        (void)vertex_glsl; (void)vertex_glsl_size;
        (void)geometry_glsl; (void)geometry_glsl_size;
        (void)fragment_glsl; (void)fragment_glsl_size;

        const void* vertex_code = vertex_spirv;
        u32 vertex_code_size = vertex_spirv_size;
        const void* fragment_code = fragment_spirv;
        u32 fragment_code_size = fragment_spirv_size;

        MetalContext* ctx = GetMetalContext();

        // Create shader library from source code (Metal shaders are text-based)
        NSString* vertexSource = [[NSString alloc] initWithBytes:vertex_code
                                                          length:vertex_code_size
                                                        encoding:NSUTF8StringEncoding];
        NSString* fragmentSource = [[NSString alloc] initWithBytes:fragment_code
                                                            length:fragment_code_size
                                                          encoding:NSUTF8StringEncoding];

        NSError* error = nil;

        // Compile vertex shader
        id<MTLLibrary> vertexLibrary = [ctx->device newLibraryWithSource:vertexSource
                                                                 options:nil
                                                                   error:&error];
        if (!vertexLibrary) {
            NSLog(@"Failed to compile vertex shader: %@", error);
            return nullptr;
        }

        // Compile fragment shader
        id<MTLLibrary> fragmentLibrary = [ctx->device newLibraryWithSource:fragmentSource
                                                                   options:nil
                                                                     error:&error];
        if (!fragmentLibrary) {
            NSLog(@"Failed to compile fragment shader: %@", error);
            return nullptr;
        }

        id<MTLFunction> vertexFunction = [vertexLibrary newFunctionWithName:@"vertexMain"];
        id<MTLFunction> fragmentFunction = [fragmentLibrary newFunctionWithName:@"fragmentMain"];

        if (!vertexFunction || !fragmentFunction) {
            NSLog(@"Failed to find shader entry points");
            return nullptr;
        }

        // Create pipeline state
        MTLRenderPipelineDescriptor* pipelineDesc = [MTLRenderPipelineDescriptor new];
        pipelineDesc.vertexFunction = vertexFunction;
        pipelineDesc.fragmentFunction = fragmentFunction;
        pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

        // Enable blending if needed
        if (flags & SHADER_FLAGS_BLEND) {
            pipelineDesc.colorAttachments[0].blendingEnabled = YES;
            pipelineDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
            pipelineDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
            pipelineDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
            pipelineDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
            pipelineDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            pipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        }

        if (name) {
            pipelineDesc.label = [NSString stringWithUTF8String:name];
        }

        id<MTLRenderPipelineState> pipelineState = [ctx->device newRenderPipelineStateWithDescriptor:pipelineDesc
                                                                                                error:&error];
        if (!pipelineState) {
            NSLog(@"Failed to create pipeline state: %@", error);
            return nullptr;
        }

        // Create platform shader
        Shader* shader = (Shader*)Alloc(ALLOCATOR_DEFAULT, sizeof(Shader));
        shader->pipeline_state = pipelineState;
        shader->vertex_function = vertexFunction;
        shader->fragment_function = fragmentFunction;
        shader->flags = flags;

        return shader;
    }
}

void platform::DestroyShader(Shader* shader)
{
    if (shader) {
        shader->pipeline_state = nil;
        shader->vertex_function = nil;
        shader->fragment_function = nil;
        Free(shader);
    }
}

void platform::BindShader(Shader* shader)
{
    @autoreleasepool {
        g_renderer.current_shader = shader;

        MetalContext* ctx = GetMetalContext();
        if (ctx->render_encoder && shader) {
            [ctx->render_encoder setRenderPipelineState:shader->pipeline_state];
        }
    }
}

// Initialize the renderer (called from InitMetal)
void InitializeMetalRenderer()
{
    @autoreleasepool {
        MetalContext* ctx = GetMetalContext();

        // Create dynamic uniform buffers
        for (int i = 0; i < UNIFORM_BUFFER_COUNT; i++) {
            g_renderer.uniform_buffers[i].buffer = [ctx->device newBufferWithLength:DYNAMIC_UNIFORM_BUFFER_SIZE
                                                                             options:MTLResourceStorageModeShared];
            g_renderer.uniform_buffers[i].size = DYNAMIC_UNIFORM_BUFFER_SIZE;
            g_renderer.uniform_buffers[i].offset = 0;
            g_renderer.uniform_buffer_offsets[i] = 0;

            if (i < (int)(sizeof(UNIFORM_BUFFER_NAMES) / sizeof(const char*))) {
                g_renderer.uniform_buffers[i].buffer.label = [NSString stringWithUTF8String:UNIFORM_BUFFER_NAMES[i]];
            }
        }

        g_renderer.frame_index = 0;
        g_renderer.current_shader = nullptr;
        g_renderer.clear_color = COLOR_BLACK;
    }
}
