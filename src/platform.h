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
struct RectInt;

namespace platform {
    struct Buffer;
    struct BufferMemory {};
    struct Shader;
    struct Image {};
    struct ImageView {};
    struct Memory {};
    struct Sampler {};
    struct Texture;
    struct Sound {};
    struct SoundHandle { u64 value;};
    struct HttpHandle { u64 value; };
    struct WebSocketHandle { u64 value; };

    enum class HttpStatus
    {
        None,       // No request or invalid handle
        Pending,    // Request in progress
        Complete,   // Request completed successfully
        Error       // Request failed
    };

    enum class WebSocketStatus
    {
        None,
        Connecting,
        Connected,
        Closing,
        Closed,
        Error
    };

    enum class WebSocketMessageType
    {
        Text,
        Binary
    };

    // @app
    void InitApplication(const ApplicationTraits* traits);
    void ShutdownApplication();
    void InitWindow(void (*on_close)());
    void FocusWindow();
    bool UpdateApplication();
    Vec2Int GetScreenSize();
    void SetCursor(SystemCursor cursor);
    bool HasFocus();
    bool IsResizing();
    void Log(LogType type, const char* message);
    noz::RectInt GetWindowRect();

    // @render
    void BeginRenderFrame();
    void EndRenderFrame();
    void BeginRenderPass(Color clear_color);
    void EndRenderPass();

    // @postprocess
    void SetPostProcessEnabled(bool enabled);
    void BeginPostProcessPass();
    void EndPostProcessPass();
    void BeginUIPass();  // Begin UI render pass to ui_offscreen with MSAA
    void EndSwapchainPass();    // End UI render pass and composite onto swapchain
    void BindOffscreenTexture();
    void BindUIOffscreenTexture();
    void SetViewport(const noz::Rect& viewport); // Set viewport rect in screen pixels. Pass empty rect to reset to full screen.
    void BindTransform(const Mat3& transform, float depth=0.0f, float depth_scale=1.0f);
    void BindVertexUserData(const u8* data, u32 size);
    void BindFragmentUserData(const u8* data, u32 size);
    void BindCamera(const Mat3& view_matrix);
    void BindColor(const Color& color, const Vec2& color_uv_offset, const Color& emission);
    void BindSkeleton(const Mat3* bone_transforms, u8 bone_count);
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
        const void* vertex,
        u32 vertex_size,
        const void* geometry,
        u32 geometry_size,
        const void* fragment,
        u32 fragment_size,
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
    bool IsGamepadActive();
    bool IsMouseOverWindow();

    // @filesystem
    std::filesystem::path GetSaveGamePath();

    // @audio
    void InitializeAudio();
    void ShutdownAudio();
    Sound* CreateSound(void* data, u32 data_size, u32 sample_rate, u32 channels, u32 bits_per_sample);
    void DestroySound(Sound*);
    SoundHandle PlaySound(Sound* sound, float volume, float pitch, bool loop);
    void PlayMusic(Sound* sound);
    bool IsMusicPlaying();
    void StopMusic();
    void StopSound(const SoundHandle& handle);
    void SetSoundVolume(const SoundHandle& handle, float volume);
    void SetSoundPitch(const SoundHandle& handle, float pitch);
    bool IsSoundPlaying(const SoundHandle& handle);
    float GetSoundVolume(const SoundHandle& handle);
    float GetSoundPitch(const SoundHandle& handle);
    void SetMasterVolume(float volume);
    void SetSoundVolume(float volume);
    void SetMusicVolume(float volume);
    float GetMasterVolume();
    float GetSoundVolume();
    float GetMusicVolume();

    // @thread
    void SetThreadName(const char* name);

    // @http
    void InitializeHttp();
    void ShutdownHttp();
    HttpHandle HttpGet(const char* url);
    HttpHandle HttpPost(const char* url, const void* body, u32 body_size, const char* content_type = nullptr);
    HttpStatus HttpGetStatus(const HttpHandle& handle);
    int HttpGetStatusCode(const HttpHandle& handle);      // HTTP status code (200, 404, etc.)
    const u8* HttpGetResponse(const HttpHandle& handle, u32* out_size);
    void HttpCancel(const HttpHandle& handle);            // Cancel pending request
    void HttpRelease(const HttpHandle& handle);           // Release completed request resources

    // @websocket
    void InitializeWebSocket();
    void ShutdownWebSocket();
    void UpdateWebSocket();
    WebSocketHandle WebSocketConnect(const char* url);
    void WebSocketSend(const WebSocketHandle& handle, const char* text);
    void WebSocketSendBinary(const WebSocketHandle& handle, const void* data, u32 size);
    void WebSocketClose(const WebSocketHandle& handle, u16 code, const char* reason);
    void WebSocketRelease(const WebSocketHandle& handle);
    WebSocketStatus WebSocketGetStatus(const WebSocketHandle& handle);
    bool WebSocketHasMessage(const WebSocketHandle& handle);
    bool WebSocketGetMessage(const WebSocketHandle& handle, WebSocketMessageType* out_type, const u8** out_data, u32* out_size);
    void WebSocketPopMessage(const WebSocketHandle& handle);
}
