//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <vector>

enum InputCode;

namespace platform
{
    struct Window {};
    struct Pipeline {};
    struct PipelineLayout {};
    struct Buffer {};
    struct BufferMemory {};
    struct Shader;
    struct Image {};
    struct ImageView {};
    struct Memory {};
    struct Sampler {};
    struct Texture;

    Vec2 GetMousePosition();
    Vec2 GetCachedMousePosition();
    
    // @window
    Window* CreatePlatformWindow(const ApplicationTraits* traits);
    void DestroyPlatformWindow(Window* window);
    bool ProcessWindowEvents(Window* window, bool& has_focus, Vec2Int& screen_size);
    Vec2Int GetWindowSize(Window* window);
    void ShowCursor(bool show);
    
    // @render
    void SetRendererWindow(Window* window);
    void BeginRenderFrame();
    void EndRenderFrame();
    void BeginRenderPass(Color clear_color);
    void EndRenderPass();
    void BindTransform(const RenderTransform* transform);
    void BindCamera(const RenderCamera* camera);
    void BindBoneTransforms(const RenderTransform* bones, int count);
    void BindLight(const void* light);
    void BindColor(const void* color);
    Buffer* CreateVertexBuffer(const MeshVertex* vertices, size_t vertex_count, const char* name = nullptr);
    Buffer* CreateIndexBuffer(const uint16_t* indices, size_t index_count, const char* name = nullptr);
    void BindVertexBuffer(Buffer* buffer);
    void BindIndexBuffer(Buffer* buffer);
    void DrawIndexed(size_t index_count);
    void BindTexture(Texture* texture, int slot);
    
    // @texture
    Texture* CreateTexture(void* data, size_t width, size_t height, int channels, const SamplerOptions& sampler_options, const char* name);
    void DestroyTexture(Texture* texture);

    // @shader
    Shader* CreateShader(
        const void* vertex_code,
        u32 vertex_code_size,
        const void* fragment_code,
        u32 fragment_code_size,
        ShaderFlags flags,
        const char* name = nullptr);
    void DestroyShader(Shader* module);
    void BindShader(Shader* shader);

    // @time
    u64 GetPerformanceCounter();
    u64 GetPerformanceFrequency();
    
    // @input
    bool IsInputButtonDown(InputCode code);
    float GetInputAxisValue(InputCode code);
    void UpdateInputState();
    void InitializeInput();
    void ShutdownInput();
}

