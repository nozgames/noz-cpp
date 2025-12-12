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

struct PlatformBuffer;
struct PlatformBufferMemory {};
struct PlatformShader;
struct PlatformTexture;
struct PlatformSound {};
struct PlatformSoundHandle { u64 value;};
struct PlatformHttpHandle { u64 value; };
struct PlatformWebSocketHandle { u64 value; };

struct NativeTextInputStyle {
    Color background_color;
    Color text_color;
    int font_size;
    bool password;
};

// @platform
extern void PlatformInit(const ApplicationTraits* traits);
extern void PlatformShutdown();
extern bool PlatformUpdate();
extern void PlatformLog(LogType type, const char* message);
extern std::filesystem::path PlatformGetSaveGamePath();
extern std::filesystem::path PlatformGetBinaryPath();
extern std::filesystem::path PatformGetCurrentPath();
extern u64 PlatformGetTimeCounter();
extern u64 PlatformGetTimeFrequency();

// @thread
extern void PlatformSetThreadName(const char* name);

// @window
extern void PlatformInitWindow(void (*on_close)());
extern void PlatformFocusWindow();
extern bool PlatformIsWindowFocused();
extern bool PlatformIsWindowResizing();
extern noz::RectInt PlatformGetWindowRect();
extern Vec2Int PlatformGetWindowSize();
extern void PlatformSetCursor(SystemCursor cursor);

// @render
extern void PlatformBeginRender();
extern void PlatformEndRender();
extern void PlatformBeginScenePass(Color clear_color);
extern void PlatformEndScenePass();
extern void PlatformFree(PlatformBuffer* buffer);
extern void PlatformBindVertexBuffer(PlatformBuffer* buffer);
extern void PlatformBindIndexBuffer(PlatformBuffer* buffer);
extern void PlatformBindSkeleton(const Mat3* bone_transforms, u8 bone_count);
extern PlatformTexture* PlatformCreateTexture(
    void* data,
    size_t width,
    size_t height,
    int channels,
    const SamplerOptions& sampler_options,
    const char* name);
extern void PlatformFree(PlatformTexture* texture);
extern void PlatformEnablePostProcess(bool enabled);
extern void PlatformBeginPostProcPass();
extern void PlatformEndPostProcPass();
extern void PlatformBeginUIPass();
extern void PlatformEndUIPass();
extern void PlatformBeginCompositePass();
extern void PlatformEndCompositePass();
extern void PlatformBindSceneTexture();
extern void PlatformBindUITexture();
extern void PlatformSetViewport(const noz::Rect& viewport);
extern void PlatformBindTransform(const Mat3& transform, float depth, float depth_scale);
extern void PlatformBindVertexUserData(const u8* data, u32 size);
extern void PlatformBindFragmentUserData(const u8* data, u32 size);
extern void PlatformBindCamera(const Mat3& view_matrix);
extern void PlatformBindColor(const Color& color, const Vec2& color_uv_offset, const Color& emission);
extern PlatformBuffer* PlatformCreateVertexBuffer(const MeshVertex* vertices, u16 vertex_count, const char* name);
extern PlatformBuffer* PlatformCreateIndexBuffer(const u16* indices, u16 index_count, const char* name);
extern void PlatformDrawIndexed(u16 index_count);
extern void PlatformBindTexture(PlatformTexture* texture, int slot);
extern PlatformShader* PlatformCreateShader(
    const void* vertex,
    u32 vertex_size,
    const void* fragment,
    u32 fragment_size,
    ShaderFlags flags,
    const char* name = nullptr);
extern void PlatformFree(PlatformShader* shader);
extern void PlatformBindShader(PlatformShader* shader);

// @websocket
extern void PlatformInitWebSocket();
extern void PlatformShutdownWebSocket();
extern void PlatfrormUpdateWebSocket();
extern PlatformWebSocketHandle PlatformConnectWebSocket(const char* url);
extern void PlatformSend(const PlatformWebSocketHandle& handle, const char* text);
extern void PlatformSendBinary(const PlatformWebSocketHandle& handle, const void* data, u32 size);
extern void PlatformClose(const PlatformWebSocketHandle& handle, u16 code, const char* reason);
extern void PlatformFree(const PlatformWebSocketHandle& handle);
extern WebSocketStatus PlatformGetStatus(const PlatformWebSocketHandle& handle);
extern bool PlatformHasMessages(const PlatformWebSocketHandle& handle);
extern bool PlatformGetMessage(const PlatformWebSocketHandle& handle, WebSocketMessageType* out_type, const u8** out_data, u32* out_size);
extern void PlatformPopMessage(const PlatformWebSocketHandle& handle);

// @input
extern bool PlatformIsInputButtonDown(InputCode code);
extern float PlatformGetInputAxisValue(InputCode code);
extern void PlatformUpdateInputState();
extern void PlatformInitInput();
extern void PlatformShutdownInput();
extern Vec2 PlatformGetMousePosition();
extern Vec2 PlatformGetMouseScroll();
extern const TextInput& PlatformGetTextInput();
extern void PlatformClearTextInput();
extern void PlatformSetTextInput(const TextInput& text_input);
extern bool PlatformIsGamepadActive();
extern bool PlatformIsMouseOverWindow();

// @native_text_input
extern void PlatformShowTextbox(const noz::Rect& rect, const char* initial_value, const NativeTextInputStyle& style);
extern void PlatformHideTextbox();
extern void PlatformUpdateTextbox(const noz::Rect& rect);
extern bool PlatformIsTextboxVisible();
extern void PlatformGetTextboxValue(Text& text);

// @audio
extern void PlatformInitAudio();
extern void PlatformShutdownAudio();
extern PlatformSound* PlatformCreateSound(void* data, u32 data_size, u32 sample_rate, u32 channels, u32 bits_per_sample);
extern void PlatformFree(PlatformSound*);
extern PlatformSoundHandle PlatformPlaySound(PlatformSound* sound, float volume, float pitch, bool loop);
extern void PlatformPlayMusic(PlatformSound* sound);
extern bool PlatformIsMusicPlaying();
extern void PlatformStopMusic();
extern void PlatformStopSound(const PlatformSoundHandle& handle);
extern void PlatformSetSoundVolume(const PlatformSoundHandle& handle, float volume);
extern void PlatformSetSoundPitch(const PlatformSoundHandle& handle, float pitch);
extern bool PlatformIsSoundPlaying(const PlatformSoundHandle& handle);
extern float PlatformGetSoundVolume(const PlatformSoundHandle& handle);
extern float PlatformGetSoundPitch(const PlatformSoundHandle& handle);
extern void PlatformSetMasterVolume(float volume);
extern void PlatformSetSoundVolume(float volume);
extern void PlatformSetMusicVolume(float volume);
extern float PlatformGetMasterVolume();
extern float PlatformGetSoundVolume();
extern float PlatformGetMusicVolume();

// @http
extern void PlatformInitHttp();
extern void PlatformShutdownHttp();
extern void PlatformUpdateHttp();
extern PlatformHttpHandle PlatformGetURL(const char* url);
extern PlatformHttpHandle PlatformPostURL(const char* url, const void* body, u32 body_size, const char* content_type, const char* headers, const char* method);
extern HttpStatus PlatformGetStatus(const PlatformHttpHandle& handle);
extern int PlatformGetStatusCode(const PlatformHttpHandle& handle);      // HTTP status code (200, 404, etc.)
extern const u8* PlatformGetResponse(const PlatformHttpHandle& handle, u32* out_size);
extern char* PlatformGetResponseHeader(const PlatformHttpHandle& handle, const char* name, Allocator* allocator);
extern void PlatformCancel(const PlatformHttpHandle& handle);            // Cancel pending request
extern void PlatformFree(const PlatformHttpHandle& handle);           // Release completed request resources

extern void Main();
