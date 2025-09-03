//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once


namespace platform
{
    struct Window {};
    struct Pipeline {};
    struct PipelineLayout {};

    Vec2 GetMousePosition();
    
    // @window
    Window* CreatePlatformWindow(const ApplicationTraits* traits);
    void DestroyPlatformWindow(Window* window);
    bool ProcessWindowEvents(Window* window, bool& has_focus, Vec2Int& screen_size);
    Vec2Int GetWindowSize(Window* window);
    void ShowCursor(bool show);
    
    // @render
    void SetRendererWindow(Window* window);
    Pipeline* CreatePipeline(Shader* shader, bool msaa, bool shadow);
    void BindPipeline(Pipeline* pipeline);
    void DestroyPipeline(Pipeline* pipeline);

    // @time
    u64 GetPerformanceCounter();
    u64 GetPerformanceFrequency();
}

