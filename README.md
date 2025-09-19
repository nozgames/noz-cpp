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

bool LoadAssets(Allocator* allocator) {
    // Load game assets here
    return true;
}

void UnloadAssets() {
    // Cleanup assets
}

int main() {
    ApplicationTraits traits = {
        .name = "MyGame",
        .title = "My Awesome Game",
        .width = 1920,
        .height = 1080,
        .load_assets = LoadAssets,
        .unload_assets = UnloadAssets
    };
    
    Init(traits);
    return 0;
}
```

### Rendering a Textured Quad

```cpp
// Create camera
Camera* camera = CreateCamera(allocator);
SetSize(camera, Vec2{1920, 1080});

// Load assets
Texture* texture = LoadAsset<Texture>("my_texture");
Shader* shader = LoadAsset<Shader>("lit");
Material* material = CreateMaterial(allocator, shader);
SetTexture(material, texture);

// Create mesh
MeshBuilder* builder = CreateMeshBuilder(allocator, 4, 6);
AddQuad(builder, Vec2{0, 1}, Vec2{1, 0}, 100, 100, Vec2{0, 0});
Mesh* quad = CreateMesh(allocator, builder, MakeName("quad"));

// Render
BeginRenderFrame(Color{0.1f, 0.1f, 0.1f, 1.0f});
BindCamera(camera);
BindMaterial(material);
BindTransform(Vec2{0, 0}, 0.0f, Vec2{1, 1});
DrawMesh(quad);
EndRenderFrame();
```

### UI System

```cpp
BeginUI(1920, 1080);
BeginCanvas();

Label("Hello, World!");
Image(my_material);

// Custom element with input handling
SetInputHandler([](const ElementInput& input) -> bool {
    if (input.button == INPUT_CODE_MOUSE_LEFT) {
        Log("Button clicked!");
        return true; // Consume input
    }
    return false;
});
BeginElement();
Label("Click Me");
EndElement();

EndCanvas();
DrawUI();
EndUI();
```

### Audio Playback

```cpp
Sound* sound = LoadAsset<Sound>("explosion");
SoundHandle handle = Play(sound, 1.0f, 1.0f, false);

// Control playback
SetVolume(handle, 0.5f);
SetPitch(handle, 1.2f);

if (IsPlaying(handle)) {
    Stop(handle);
}
```

### VFX System

```cpp
Vfx* explosion = LoadAsset<Vfx>("explosion_effect");
VfxHandle effect = Play(explosion, Vec2{100, 200});

// Check if still playing
if (IsPlaying(effect)) {
    // Effect is active
}
```

## 🏗️ Architecture

### Memory Management
NoZ uses custom allocators for optimal performance:
- **Arena Allocators**: For temporary/frame-based data
- **Pool Allocators**: For fixed-size objects
- **Asset Memory**: Dedicated memory pools for game assets

### Asset Pipeline
Assets are referenced by name and automatically managed:
```cpp
// Assets loaded by name without file extensions
Texture* texture = LoadAsset<Texture>("player_sprite");
Material* material = CreateMaterial(allocator, shader);
SetTexture(material, texture);
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

## 🎮 Example Projects

Check out these example projects to see NoZ in action:
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