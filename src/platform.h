//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <filesystem>

enum InputCode;
typedef u32 ShaderFlags;
struct ApplicationTraits;
struct MeshVertex;
struct SamplerOptions;

namespace platform
{
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
    struct Sound {};
    struct SoundHandle { u64 value;};

    // @app
    void InitApplication(const ApplicationTraits* traits);
    void ShutdownApplication();
    void InitWindow(void (*on_close)());
    void FocusWindow();
    bool UpdateApplication();
    Vec2Int GetScreenSize();
    void ShowCursor(bool show);
    void SetCursor(SystemCursor cursor);
    bool HasFocus();
    bool IsResizing();
    void Log(LogType type, const char* message);
    RectInt GetWindowRect();

    // @render
    void BeginRenderFrame();
    void EndRenderFrame();
    void BeginRenderPass(Color clear_color);
    void EndRenderPass();
    void BindTransform(const Mat3& transform, float depth=0.0f);
    void BindVertexUserData(const u8* data, u32 size);
    void BindFragmentUserData(const u8* data, u32 size);
    void BindLight(const Vec3& light_dir, const Color& diffuse_color, const Color& shadow_color);
    void BindCamera(const Mat3& view_matrix);
    void BindColor(const Color& color, const Vec2& color_uv_offset);
    Buffer* CreateVertexBuffer(
        const MeshVertex* vertices,
        u16 vertex_count,
        const char* name = nullptr);
    Buffer* CreateIndexBuffer(const u16* indices, u16 index_count, const char* name = nullptr);
    void DestroyBuffer(Buffer* buffer);
    void BindVertexBuffer(Buffer* buffer);
    void BindIndexBuffer(Buffer* buffer);
    void DrawIndexed(u16 index_count);
    void BindTexture(Texture* texture, int slot);

    // @texture
    Texture* CreateTexture(
        void* data,
        size_t width,
        size_t height,
        int channels,
        const SamplerOptions& sampler_options,
        const char* name);
    void DestroyTexture(Texture* texture);

    // @shader
    Shader* CreateShader(
        const void* vertex_code,
        u32 vertex_code_size,
        const void* geometry_code,
        u32 geometry_code_size,
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
    Vec2 GetMousePosition();
    Vec2 GetMouseScroll();
    const TextInput& GetTextInput();
    void ClearTextInput();
    extern void SetTextInput(const TextInput& text_input);

    // @filesystem
    std::filesystem::path GetSaveGamePath();

    // @audio
    void InitializeAudio();
    void ShutdownAudio();
    Sound* CreateSound(void* data, u32 data_size, u32 sample_rate, u32 channels, u32 bits_per_sample);
    void DestroySound(Sound*);
    SoundHandle PlaySound(Sound* sound, float volume, float pitch, bool loop);
    void StopSound(const SoundHandle& handle);
    void SetSoundVolume(const SoundHandle& handle, float volume);
    void SetSoundPitch(const SoundHandle& handle, float pitch);
    bool IsSoundPlaying(const SoundHandle& handle);
    float GetSoundVolume(const SoundHandle& handle);
    float GetSoundPitch(const SoundHandle& handle);
    void SetMasterVolume(float volume);
    float GetMasterVolume();

    // @thread
    void SetThreadName(const char* name);
}
