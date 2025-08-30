//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

constexpr u64 RENDER_MASK_DEFAULT = 1 << 0;

// @entities
struct Entity : Object { };
struct Camera : Entity { };

// @components
struct Component : Object { };
struct MeshRenderer : Component { };

// @entity
#define ENTITY_BASE_SIZE 272
#define ENTITY_BASE EntityBase __entity;

struct EntityBase { u8 _entity[ENTITY_BASE_SIZE]; };

struct EntityTraits
{
    void(*on_destroy)(Entity*);
    void(*on_enabled)(Entity*);
    void(*on_disabled)(Entity*);

#ifdef NOZ_EDITOR
    void(*editor_inspect)(Entity*, Stream*);
#endif
};

extern const EntityTraits* g_entity_traits[];

void SetEntityTraits(type_t id, const EntityTraits* traits);
inline const EntityTraits* GetEntityTraits(type_t id) { return g_entity_traits[id]; }
inline const EntityTraits* GetEntityTraits(Entity* entity) { return g_entity_traits[GetType(entity)]; }

Entity* GetRootEntity();
Entity* CreateEntity(Allocator* allocator, size_t entity_size, type_t type_id);
Entity* CreateEntity(Allocator* allocator);
void SetEnabled(Entity* entity, bool enabled);
bool IsEnabled(Entity* entity);
void SetParent(Entity* entity, Entity* parent);
void RemoveFromParent(Entity* entity);
void AddComponent(Entity* entity, Component* component);
void RemoveComponent(Entity* entity, Component* component);
Component* GetComponent(Entity* entity, type_t component_type);
Entity* GetParent(Entity* entity);
Entity* GetFirstChild(Entity* entity);
Entity* GetNextChild(Entity* entity, Entity* child);
Entity* GetPrevChild(Entity* entity, Entity* child);
int GetChildCount(Entity* entity);
vec3 GetWorldPosition(Entity* entity);
vec3 GetLocalPosition(Entity* entity);
const mat4& GetWorldToLocal(Entity* entity);
const mat4& GetLocalToWorld(Entity* entity);
vec3 InverseTransformPoint(Entity* entity, const vec3& point);
void SetLocalPosition(Entity* entity, const vec3& pos);
inline void SetLocalPosition(Entity* entity, float x, float y, float z) { SetLocalPosition(entity, vec3(x,y,z));}
void SetLocalScale(Entity* entity, const vec3& scale);
void SetLocalScale(Entity* entity, float scale);
void SetWorldPosition(Entity* e, float x, float y, float z);
void SetWorldPosition(Entity* e, const vec3& world_position);
void SetLocalEulerAngles(Entity* entity, const vec3& angles_in_degrees);
inline void SetLocalEulerAngles(Entity* entity, float x_in_degrees, float y_in_degrees, float z_in_degrees)
{

    SetLocalEulerAngles(entity, vec3(x_in_degrees, y_in_degrees, z_in_degrees));
}
void SetLocalRotation(Entity* entity, const quat& rotation);
void SetWorldRotation(Entity* entity, const quat& rotation);
void SetWorldEulerAngles(Entity* entity, const vec3& angles);
inline void SetWorldEulerAngles(Entity* entity, float x, float y, float z)
{
    SetWorldEulerAngles(entity, vec3(x, y, z));
}
void BindTransform(Entity* entity);
void LookAt(Entity* entity, const vec3& target, const vec3& up);

// @component
#define COMPONENT_BASE_SIZE 40
#define COMPONENT_BASE ComponentBase _component;

struct ComponentTraits
{
    void(*on_enter_entity)(Component*, Entity*);
    void(*on_leave_entity)(Component*, Entity*);
    void(*on_enabled)(Component*);
    void(*on_disabled)(Component*);
};

extern const ComponentTraits* g_component_traits[];

struct ComponentBase { u8 _component[COMPONENT_BASE_SIZE]; };

void SetComponentTraits(type_t id, const ComponentTraits* traits);
inline const ComponentTraits* GetComponentTraits(type_t id) { return g_component_traits[id]; }
inline const ComponentTraits* GetComponentTraits(Component* component) { return g_component_traits[GetType(component)]; }

Component* CreateComponent(Allocator* allocator, size_t component_size, type_t component_type);
Entity* GetEntity(Component* component);

// @camera
Camera* CreateCamera(Allocator* allocator);
void Render(Camera* camera);
void SetRenderLayerMask(Camera* camera, u64 mask);
u64 GetRenderLayerMask(Camera* camera);
const mat4& GetProjection(Camera* camera);
void SetOrthographic(Camera* camera, float left, float right, float top, float bottom, float near, float far);
void SetOrthographic(Camera* camera, float view_height, float near, float far);
bool IsOrthographic(Camera* camera);
vec3 ScreenToWorld(Camera* camera, const vec2& screen_pos);
vec2 WorldToScreen(Camera* camera, const vec3& world_pos);

// @mesh_renderer
MeshRenderer* CreateMeshRenderer(Allocator* allocator);
void SetMesh(MeshRenderer* renderer, Mesh* mesh);
void SetMaterial(MeshRenderer* renderer, Material* material);
void SetRenderLayer(MeshRenderer* renderer, uint64_t layer);

