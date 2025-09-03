//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once


namespace platform
{
    struct Window {};
    struct Pipeline {};
    struct PipelineLayout {};
    struct Buffer {};
    struct BufferMemory {};
    struct ShaderModule {};

    Vec2 GetMousePosition();
    
    // @window
    Window* CreatePlatformWindow(const ApplicationTraits* traits);
    void DestroyPlatformWindow(Window* window);
    bool ProcessWindowEvents(Window* window, bool& has_focus, Vec2Int& screen_size);
    Vec2Int GetWindowSize(Window* window);
    void ShowCursor(bool show);
    
    // @render
    void SetRendererWindow(Window* window);
    Pipeline* CreatePipeline(Shader* shader, bool msaa);
    void BindPipeline(Pipeline* pipeline);
    void DestroyPipeline(Pipeline* pipeline);
    void BeginRenderFrame();
    void EndRenderFrame();
    void BeginRenderPass(Color clear_color);
    void EndRenderPass();
    void BindTransform(const RenderTransform* transform);
    void BindCamera(const RenderCamera* camera);
    void BindBoneTransforms(const RenderTransform* bones, int count);
    void BindLight(const void* light);
    void BindColor(const void* color);
    Buffer* CreateVertexBuffer(const MeshVertex* vertices, size_t vertex_count);
    Buffer* CreateIndexBuffer(const uint16_t* indices, size_t index_count);
    void BindVertexBuffer(Buffer* buffer);
    void BindIndexBuffer(Buffer* buffer);
    void DrawIndexed(size_t index_count);
    
    // Shader functions
    ShaderModule* CreateShaderModule(const void* spirv_code, size_t code_size);
    void DestroyShaderModule(ShaderModule* module);

    // @time
    u64 GetPerformanceCounter();
    u64 GetPerformanceFrequency();
}

