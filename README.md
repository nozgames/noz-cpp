# NoZ Game Engine

**NoZ** is a high-performance C++ game engine designed for 2D game development with modern graphics capabilities. Built with a focus on simplicity, performance, and clean C-style APIs, NoZ provides everything needed to create compelling 2D games and interactive applications.

## 🎯 Features

### Core Engine
- **C-Style API**: Clean, simple function-based interface with minimal C++ complexity
- **Custom Memory Management**: Arena and pool allocators for optimal performance
- **Asset System**: Hot-reloadable assets with automatic dependency tracking
- **Job System**: Multi-threaded task execution with dependency management
- **Event System**: Decoupled communication between engine systems
- **Editor Integration**: Real-time asset hot-reloading during development

### Graphics & Rendering
- **Vulkan Backend**: Modern, cross-platform graphics API with optimized rendering
- **Material System**: Shader-based materials with multi-texture support
- **Mesh System**: Dynamic mesh building and efficient static mesh rendering
- **Camera System**: 2D orthographic cameras with world-to-screen transformations
- **Animation System**: Skeletal animation with bone hierarchies and keyframe interpolation
- **Text Rendering**: High-quality SDF-based font rendering with kerning support
- **Built-in Shaders**: Optimized shaders for common rendering tasks (lit, UI, text, VFX)

### Audio
- **XAudio2 Integration**: High-performance audio playback on Windows
- **Sound Management**: Easy-to-use sound playback with volume and pitch control
- **Handle-based API**: Safe sound instance management with automatic cleanup

### User Interface
- **Immediate Mode UI**: Simple, declarative UI system for in-game interfaces
- **Flexbox Layout**: CSS-like layout system with flexible positioning
- **Style Sheets**: External styling support for consistent UI appearance
- **Canvas System**: Both screen-space and world-space UI rendering
- **Input Handling**: Mouse and keyboard input with element-specific callbacks

### VFX System
- **Particle Effects**: Asset-based visual effects system
- **Handle Management**: Safe VFX instance tracking and control
- **Spatial Integration**: World-positioned effects with automatic bounds calculation

### Physics (Simplified)
- **Collision Detection**: Point-in-polygon and line intersection testing
- **Raycasting**: Spatial queries for gameplay mechanics
- **Custom Colliders**: Mesh-based and primitive shape colliders

### Platform Support
- **Windows**: Primary platform with full feature support
- **Vulkan Graphics**: Cross-platform graphics foundation
- **C++23**: Modern C++ features with backwards compatibility

## 🚀 Quick Start

### Basic Application Setup

```cpp
#include <noz/noz.h>

extern bool LoadAssets(Allocator* allocator);
extern void UnloadAssets();
extern void InitGame(int argc, const char** argv);
extern void RunGame();
extern void ShutdownGame();

// Main entry point
int main(int argc, char** argv) {
    InitGame(argc, (const char**)argv);
    RunGame();
    ShutdownGame();
    return 0;
}

// Game initialization
void InitGame(int argc, const char** argv) {
    ApplicationTraits traits = {};
    Init(traits);
    traits.name = "MyGame";
    traits.title = "My Awesome Game";
    traits.load_assets = LoadAssets;
    traits.unload_assets = UnloadAssets;
    traits.editor_port = 8081;
#ifdef NOZ_EDITOR
    traits.hotload_asset = HotloadAsset;
#endif
    traits.renderer.msaa = true;
    InitApplication(&traits, argc, argv);
    InitWindow();

    // Initialize game systems here
}
```

### Asset Loading and Rendering

```cpp
// Assets are loaded by name using global constants (auto-generated)
bool LoadAssets(Allocator* allocator) {
    // Load textures, shaders, meshes, sounds, etc.
    NOZ_LOAD_TEXTURE(allocator, PATH_TEXTURE_PALETTE, TEXTURE_PALETTE);
    NOZ_LOAD_SHADER(allocator, PATH_SHADER_LIT, SHADER_LIT);
    NOZ_LOAD_MESH(allocator, PATH_MESH_GROUND, MESH_GROUND);
    NOZ_LOAD_SOUND(allocator, PATH_SOUND_BOW_FIRE, SOUND_BOW_FIRE);
    return true;
}

// Create camera and materials in initialization
Camera* camera = CreateCamera(ALLOCATOR_DEFAULT);
SetExtents(camera, WORLD_LEFT, WORLD_RIGHT, WORLD_BOTTOM, -F32_MAX);

Material* material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_LIT);
SetTexture(material, TEXTURE_PALETTE, 0);

// Render in game loop
BeginRenderFrame({0.3f, 0.3f, 0.9f, 1.0f});
BindCamera(camera);
BindLight(Normalize(Vec3{1, 1, 0}), COLOR_WHITE, COLOR_BLACK);
BindMaterial(material);
BindTransform(Vec2{0, 0}, 0.0f, Vec2{1, 1});
DrawMesh(MESH_GROUND);
EndRenderFrame();
```

### UI System with Stylesheets

```cpp
// Load stylesheets in LoadAssets
NOZ_LOAD_STYLESHEET(allocator, PATH_STYLESHEET_MENU, STYLESHEET_MENU);

// UI rendering with styles (in game loop)
BeginUI(UI_REF_WIDTH, UI_REF_HEIGHT);

// Menu with styled elements
BeginCanvas(STYLE_MENU_BACKGROUND);
    BeginElement(STYLE_MENU_BUTTONS);
        BeginElement(STYLE_MENU_LARGE_BUTTON);
            SetInputHandler(HandlePlayButton, nullptr);
            Label("PLAY", STYLE_MENU_LARGE_BUTTON_TEXT);
        EndElement();
        BeginElement(STYLE_MENU_LARGE_BUTTON);
            SetInputHandler(HandleQuitButton, nullptr);
            Label("QUIT", STYLE_MENU_LARGE_BUTTON_TEXT);
        EndElement();
    EndElement();
EndCanvas();

DrawUI();
EndUI();

// Input handler example
static bool HandlePlayButton(const ElementInput& input) {
    g_game.state = GAME_STATE_PLAYING;
    return true;
}
```

### Audio Playback

```cpp
// Load sounds in LoadAssets function
NOZ_LOAD_SOUND(allocator, PATH_SOUND_BOW_FIRE, SOUND_BOW_FIRE);
NOZ_LOAD_SOUND(allocator, PATH_SOUND_ARROW_IMPACT, SOUND_ARROW_IMPACT);

// Play sounds with volume and pitch control
SoundHandle charge_sound = Play(SOUND_BOW_ARROW_DRAW_B, 0.25f, 1.0f);
Play(SOUND_BOW_ARROW_FIRE_A, 0.5f);  // Fire and forget
Play(SOUND_BULLET_IMPACT_B, 0.5f);

// Control ongoing sound playback
if (IsPlaying(charge_sound)) {
    Stop(charge_sound);
}
SetVolume(charge_sound, 0.8f);
```

### VFX System

```cpp
// Load VFX in LoadAssets function
NOZ_LOAD_VFX(allocator, PATH_VFX_ARROW_HIT, VFX_ARROW_HIT);
NOZ_LOAD_VFX(allocator, PATH_VFX_BOW_FIRE, VFX_BOW_FIRE);

// Play VFX at world positions
Play(VFX_ARROW_HIT, target_position);
Play(VFX_BOW_FIRE, bow_position + bow_direction * 0.5f);

// VFX are automatically managed and cleaned up
// Draw all active VFX in render loop
DrawVfx();
```

## 🏗️ Architecture

### Memory Management
NoZ uses custom allocators for optimal performance:
- **Arena Allocators**: For temporary/frame-based data
- **Pool Allocators**: For fixed-size objects
- **Asset Memory**: Dedicated memory pools for game assets

### Asset Pipeline
Assets are auto-generated as global constants and loaded via macros:
```cpp
// Auto-generated in game_assets.h
extern Texture* TEXTURE_PALETTE;
extern Sound* SOUND_BOW_FIRE;
extern Mesh* MESH_GROUND;

// Loading in LoadAssets function
NOZ_LOAD_TEXTURE(allocator, PATH_TEXTURE_PALETTE, TEXTURE_PALETTE);
NOZ_LOAD_SOUND(allocator, PATH_SOUND_BOW_FIRE, SOUND_BOW_FIRE);
NOZ_LOAD_MESH(allocator, PATH_MESH_GROUND, MESH_GROUND);

// Usage throughout the game
Material* material = CreateMaterial(allocator, SHADER_LIT);
SetTexture(material, TEXTURE_PALETTE, 0);
```

### Handle-Based APIs
Audio and VFX systems use handles for safe resource management:
```cpp
SoundHandle sound = Play(audio_clip);
VfxHandle effect = Play(particle_system, position);
// Handles automatically become invalid when resources are freed
```

## 📁 Project Structure

```
noz/
├── include/noz/          # Public API headers
│   ├── noz.h            # Main include file
│   ├── renderer.h       # Graphics and rendering
│   ├── audio.h          # Audio system
│   ├── ui.h             # User interface
│   ├── vfx.h            # Visual effects
│   └── ...
├── src/                 # Implementation files
│   ├── render/          # Rendering system
│   ├── audio/           # Audio implementation
│   ├── ui/              # UI system
│   ├── vfx/             # VFX system
│   ├── math/            # Math utilities
│   ├── memory/          # Memory management
│   └── platform/        # Platform-specific code
├── assets/              # Engine assets
│   └── shaders/         # Built-in shaders
├── external/            # Third-party dependencies
└── CMakeLists.txt       # Build configuration
```

## 🔧 Building

### Requirements
- **CMake 3.15+**
- **C++23 compatible compiler** (MSVC, GCC, Clang)
- **Vulkan SDK** (for graphics)
- **Git** (for dependency management)

### Build Steps
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Dependencies
NoZ includes the following third-party libraries:
- **Vulkan** - Modern graphics API
- **XAudio2** - Audio system (Windows)
- **ENet** - Networking (optional)
- **xxHash** - Fast hashing
- **STB Libraries** - Image loading and utilities

### Input System
The engine uses input sets for managing different input contexts:

```cpp
// Create input sets for different game states
InputSet* game_input = CreateInputSet(ALLOCATOR_DEFAULT);
EnableButton(game_input, KEY_A);
EnableButton(game_input, KEY_D);
EnableButton(game_input, KEY_SPACE);
EnableButton(game_input, MOUSE_LEFT);
EnableButton(game_input, KEY_ESCAPE);
SetInputSet(game_input);

InputSet* ui_input = CreateInputSet(ALLOCATOR_DEFAULT);
EnableButton(ui_input, KEY_ESCAPE);
EnableButton(ui_input, KEY_SPACE);

// Switch input contexts
if (game_state == GAME_STATE_PAUSED) {
    PushInputSet(ui_input);  // Add UI input on top
} else {
    PopInputSet();  // Remove UI input layer
}

// Check input in update loop
if (WasButtonPressed(game_input, KEY_ESCAPE)) {
    OpenPauseMenu();
}
```

### Animation System
Play skeletal animations on characters and objects:

```cpp
// Load animations and skeletons in LoadAssets
NOZ_LOAD_ANIMATION(allocator, PATH_ANIMATION_ARCHER_IDLE, ANIMATION_ARCHER_IDLE);
NOZ_LOAD_SKELETON(allocator, PATH_SKELETON_ARCHER, SKELETON_ARCHER);

// Initialize animator
Animator animator;
Init(animator, SKELETON_ARCHER);

// Play animations
Play(animator, ANIMATION_ARCHER_IDLE, 1.0f, true);  // Loop idle animation
Play(animator, ANIMATION_ARCHER_RUN, 1.0f, true);   // Switch to run animation

// Update and draw in game loop
Update(animator);
Draw(animator, world_transform);
```

## 🎮 Example Projects

Check out these example projects to see NoZ in action:
- **NockerZ** - Complete archery game with physics, animation, and UI
- **Basic Renderer** - Simple textured quad rendering
- **UI Demo** - Comprehensive UI system showcase
- **Audio Test** - Sound playback and management
- **VFX Demo** - Particle effects and visual systems

## 📊 Performance Characteristics

- **Memory Efficient**: Custom allocators minimize allocation overhead
- **Cache Friendly**: Data structures designed for optimal memory access patterns
- **Minimal Overhead**: C-style APIs with minimal abstraction cost
- **Vulkan Optimized**: Modern graphics pipeline with efficient command submission
- **Asset Streaming**: Hot-reload system designed for fast iteration

## 🔍 API Design Philosophy

NoZ follows these design principles:
- **Explicit Memory Management**: All allocations require an allocator parameter
- **Handle-Based Resources**: Safe resource management through opaque handles
- **C-Style Functions**: Simple, predictable function interfaces
- **Minimal Dependencies**: Self-contained engine with minimal external requirements
- **Performance First**: Every API designed with performance implications in mind

## 📝 License

Copyright (c) 2025 NoZ Games, LLC

---

**NoZ Game Engine** - Built for developers who value performance, simplicity, and creative freedom.