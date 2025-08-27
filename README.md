# NoZ Game Engine

**NoZ Game Engine** is a lightweight, modern C++ game engine designed for 2D and 3D game development with a focus on simplicity, performance, and clean architecture.

## Overview

NoZ provides a complete set of tools for game development including rendering, physics, UI, audio, input handling, and asset management. The engine follows a component-based architecture with an object system that provides automatic memory management through reference counting.

## Key Features

### Core Systems
- **Object System**: Reference-counted object management with automatic cleanup
- **Scene Graph**: Hierarchical entity system with transform management
- **Component Architecture**: Modular, trait-based component system
- **Asset Pipeline**: Hot-reloadable asset system with build tools
- **Memory Management**: Custom allocators (arena, pool) for performance

### Rendering
- **Modern Graphics API**: Cross-platform rendering abstraction
- **Material System**: Shader-based materials with texture support
- **Mesh System**: Dynamic mesh building and static mesh rendering
- **Text Rendering**: SDF-based font rendering with kerning support
- **Shadow Mapping**: Real-time shadow system

### Physics
- **2D Physics**: Box2D integration for rigid body dynamics
- **Collision Detection**: Shape-based colliders (box, circle)
- **Raycasting**: Spatial queries for gameplay mechanics

### User Interface
- **Flexbox Layout**: CSS-like layout system
- **Style Sheets**: External styling with pseudo-state support
- **UI Components**: Canvas, elements, labels, buttons
- **Screen/World Space**: Support for both overlay and 3D UI

### Input & Platform
- **Action Maps**: Configurable input binding system
- **Cross-Platform**: Windows and POSIX support
- **Hot Reload**: Runtime asset reloading for development

## Architecture

### Object System Design
NoZ uses a unique object system that combines the benefits of reference counting with type safety:

```cpp
// Objects are automatically managed
Entity* entity = CreateEntity(allocator);
// No manual cleanup required - objects are reference counted

// Type-safe casting
if (auto mesh_renderer = Cast<MeshRenderer>(entity)) {
    SetMesh(mesh_renderer, my_mesh);
}
```

### Component-Based Entities
Entities are composed of components using trait-based dispatch:

```cpp
struct MyEntityTraits : EntityTraits {
    void render(Entity* entity, Camera* camera) override;
    void update(Entity* entity) override;
};

SetEntityTraits(MY_ENTITY_TYPE, &my_traits);
```

### Asset System
Assets are loaded by name and managed automatically:

```cpp
// Assets loaded by name without extensions
Texture* texture = LoadAsset<Texture>("my_texture");
Material* material = CreateMaterial(allocator, shader);
SetTexture(material, texture);
```

## Dependencies

- **SDL3**: Window management and input
- **GLM**: Mathematics library
- **Box2D**: 2D physics simulation
- **ENet**: Networking (optional)
- **STB Libraries**: Image loading and other utilities

## Building

NoZ uses CMake for cross-platform builds:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Requirements
- CMake 3.15+
- C++23 compatible compiler
- Git (for dependency fetching)

## Quick Start

### Basic Application Setup

```cpp
#include <noz/noz.h>

bool LoadAssets(Allocator* allocator) {
    // Load your game assets
    return true;
}

void UnloadAssets() {
    // Cleanup assets
}

int main() {
    ApplicationTraits traits = {
        .title = "My Game",
        .width = 1280,
        .height = 720,
        .load_assets = LoadAssets,
        .unload_assets = UnloadAssets
    };
    
    Init(traits);
    return 0;
}
```

### Creating Entities

```cpp
// Create scene hierarchy
Entity* root = GetSceneRoot();
Entity* player = CreateEntity(allocator);
SetParent(player, root);
SetLocalPosition(player, 0, 0, 0);

// Add renderable component
MeshRenderer* renderer = CreateMeshRenderer(allocator);
SetMesh(renderer, my_mesh);
SetMaterial(renderer, my_material);
```

### UI System

```cpp
// Create UI canvas
Canvas* canvas = CreateCanvas(allocator, CANVAS_TYPE_SCREEN, 1920, 1080);

// Create UI elements
Label* label = CreateLabel(allocator, "Hello World", MakeName("title"));
SetParent(label, GetRootElement(canvas));
```

## Directory Structure

```
libs/noz/
├── include/noz/          # Public API headers
├── src/                  # Implementation files
│   ├── render/          # Rendering system
│   ├── scene/           # Scene and entity management
│   ├── ui/              # User interface system
│   ├── physics/         # Physics integration
│   ├── input/           # Input handling
│   └── memory/          # Memory management
├── assets/              # Engine assets (shaders, etc.)
├── tools/               # Build tools and importers
└── external/            # Third-party headers
```

## Performance Considerations

- **Object Pooling**: Use for frequently created/destroyed objects
- **Memory Allocators**: Arena allocators for temporary data, pool allocators for fixed-size objects
- **Asset Batching**: Group similar assets for efficient rendering
- **Hot/Cold Data**: Separate frequently accessed data from metadata

## Development Workflow

1. **Hot Reload**: Assets automatically reload when modified during development
2. **Debug Assertions**: Extensive validation in debug builds
3. **Type Safety**: Strong typing prevents common errors
4. **RAII**: Automatic resource management eliminates memory leaks

## Code Style

NoZ follows consistent naming conventions:
- Types: `snake_case` (e.g., `render_buffer`, `mesh_builder`)
- Functions: `snake_case` (e.g., `create_entity`, `add_component`)
- Constants: `UPPER_CASE` (e.g., `MAX_ENTITIES`)
- Private members: `_prefix` (e.g., `_impl`, `_object`)

## License

Copyright(c) 2025 NoZ Games, LLC

---

For detailed API documentation and examples, see the header files in `include/noz/` and implementation examples in the WoolZ game project.