//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Entity : Object { };
struct Camera : Entity { };
struct MeshRenderer : Entity { };

#define ENTITY_BASE_SIZE 256
#define ENTITY_BASE EntityBase __entity;

struct EntityBase { u8 __entity[ENTITY_BASE_SIZE]; };

// @traits
struct EntityTraits
{
    void(*render)(Entity*, Camera*);
    void(*update)(Entity*);
    void(*on_destroy)(Entity*);
    void(*on_enabled)(Entity*);
    void(*on_disabled)(Entity*);
};

// @type
extern const EntityTraits* g_entity_traits[];

void SetEntityTraits(type_t id, const EntityTraits* traits);

inline const EntityTraits* GetEntityTraits(type_t id)
{
    return g_entity_traits[id];
}

// @scene
Entity* GetSceneRoot();
void RenderScene(Camera* camera);

// @entity
Entity* CreateEntity(Allocator* allocator, size_t entity_size, type_t type_id);
Entity* CreateEntity(Allocator* allocator);
void SetParent(Entity* entity, Entity* parent);
void RemoveFromParent(Entity* entity);
Entity* GetFirstChild(Entity* entity);
Entity* GetNextChild(Entity* entity, Entity* child);
Entity* GetPrevChild(Entity* entity, Entity* child);
int GetChildCount(Entity* entity);
vec3 GetWorldPosition(Entity* entity);
const mat4& GetWorldToLocal(Entity* entity);
const mat4& GetLocalToWorld(Entity* entity);
vec3 InverseTransformPoint(Entity* entity, const vec3& point);
void SetLocalPosition(Entity* entity, const vec3& pos);
inline void SetLocalPosition(Entity* entity, float x, float y, float z) { SetLocalPosition(entity, vec3(x,y,z));}
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

// @camera
Camera* CreateCamera(Allocator* allocator);
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
