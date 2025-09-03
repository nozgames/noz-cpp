//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "vfx_internal.h"

constexpr u16 INVALID_INDEX = 0xFFFF;
constexpr u16 MAX_PARTICLES = 10096;
constexpr u16 MAX_EMITTERS = 2024;
constexpr u16 MAX_INSTANCES = 256;
constexpr u16 MAX_MESHES = 1;

enum VfxMesh
{
    VFX_MESH_SQUARE
};

struct VfxParticle
{
    Vec2 position;
    Vec2 velocity;
    VfxCurveType rotation_curve;
    float rotation_start;
    float rotation_end;
    VfxCurveType color_curve;
    Color color_start;
    Color color_end;
    VfxCurveType size_curve;
    float size_start;
    float size_end;
    VfxCurveType speed_curve;
    float speed_start;
    float speed_end;
    float lifetime;
    float elapsed;
    float rotation;
    Vec2 gravity;
    float drag;
    Mesh* mesh;
    u16 emitter_index;
};

struct VfxEmitter
{
    const VfxEmitterDef* def;
    float rate;
    float elapsed;
    float duration;
    float accumulator;
    bool active;
    u16 instance_index;
};

struct VfxInstance
{
    Vfx* vfx;
    Vec2 position;
    int particle_count;
    u32 version;
};

struct VfxSystem
{
    PoolAllocator* emitter_pool;
    PoolAllocator* instance_pool;
    PoolAllocator* particle_pool;
    u16 emitter_count;
    u16 instance_count;
    u16 particle_count;
    Mesh* meshes[MAX_MESHES];
    Material* material;
};

static VfxSystem g_vfx = {};

static VfxInstance* CreateInstance(Vfx* vfx, const Vec2& position);
static VfxEmitter* CreateEmitter(VfxInstance* instance, const VfxEmitterDef& def);
static VfxParticle* EmitParticle(VfxEmitter* emitter);

float EvaluateCurve(VfxCurveType curve, float t)
{
    t = Max(0.0f, Min(1.0f, t));

    switch (curve)
    {
    case VFX_CURVE_TYPE_LINEAR:
        return t;

    case VFX_CURVE_TYPE_EASE_IN:
        return t * t;

    case VFX_CURVE_TYPE_EASE_OUT:
        return 1.0f - (1.0f - t) * (1.0f - t);

    case VFX_CURVE_TYPE_EASE_IN_OUT:
        if (t < 0.5f)
            return 2.0f * t * t;

        return 1.0f - 2.0f * (1.0f - t) * (1.0f - t);

    case VFX_CURVE_TYPE_QUADRATIC:
        return t * t;

    case VFX_CURVE_TYPE_CUBIC:
        return t * t * t;

    case VFX_CURVE_TYPE_SINE:
        return sinf(t * noz::PI * 0.5f);

    case VFX_CURVE_TYPE_CUSTOM:
    default:
        return t; // Fallback to linear
    }
}

static float GetRandom(const VfxFloat& v) { return RandomFloat(v.min, v.max); }
static int GetRandom(const VfxInt& v) { return RandomInt(v.min, v.max); }
static Color GetRandom(const VfxColor& v) { return Mix(v.min, v.max, RandomFloat()); }
static Vec2 GetRandom(const VfxVec2& range)
{
    return { Mix(range.min.x, range.max.x, RandomFloat()),
             Mix(range.min.y, range.max.y, RandomFloat()) };
}

static u16 GetIndex(VfxInstance* instance) { return GetIndex(g_vfx.instance_pool, instance); }
static VfxInstance* GetInstance(u16 index) { return static_cast<VfxInstance*>(GetAt(g_vfx.instance_pool, index)); }
static VfxInstance* GetInstance(VfxEmitter* emitter) { return GetInstance(emitter->instance_index); }
static VfxHandle GetHandle(VfxInstance* instance) { return { GetIndex(instance), instance->version }; }

static VfxInstance* GetInstance(const VfxHandle& handle)
{
    VfxInstance* instance = GetInstance(handle.id);
    if (instance->version != handle.version)
        return nullptr;

    return instance;
}

static VfxInstance* CreateInstance(Vfx* vfx, const Vec2& position)
{
    if (g_vfx.instance_count >= MAX_INSTANCES)
        return nullptr;

    VfxInstance* instance = (VfxInstance*)Alloc(g_vfx.instance_pool, sizeof(VfxInstance));
    assert(instance);
    instance->position = position;
    instance->vfx = vfx;

    g_vfx.instance_count++;
    return instance;
}

static VfxParticle* EmitParticle(VfxEmitter* emitter)
{
    if (g_vfx.particle_count >= MAX_PARTICLES)
        return nullptr;

    VfxParticle* p = (VfxParticle*)Alloc(g_vfx.particle_pool, sizeof(VfxParticle));
    if (nullptr == p)
        return nullptr;

    VfxInstance* instance = GetInstance(emitter);
    assert(instance);

    float angle = GetRandom(emitter->def->angle);
    Vec2 dir(cos(angle), sin(angle));

    g_vfx.particle_count++;

    const VfxParticleDef& def = emitter->def->particle_def;
    p->position = GetRandom(emitter->def->spawn) + instance->position;
    p->size_start = GetRandom(def.size.start);
    p->size_end = GetRandom(def.size.end);
    p->size_curve = def.size.type;
    p->speed_start = GetRandom(def.speed.start);
    p->speed_end = GetRandom(def.speed.end);
    p->speed_curve = def.speed.type;
    p->velocity = dir * p->speed_start;
    p->color_start = GetRandom(def.color.start);
    p->color_end = GetRandom(def.color.end);
    p->color_curve = def.color.type;
    p->lifetime = GetRandom(def.duration);
    p->elapsed = 0.0f;
    p->gravity = GetRandom(def.gravity);
    p->drag = GetRandom(def.drag);
    p->rotation_curve = def.rotation.type;
    p->rotation_start = Radians(GetRandom(def.rotation.start));
    p->rotation_end = Radians(GetRandom(def.rotation.end));
    p->rotation = p->rotation_start;

    // // Load mesh if specified (default to quad)
    // if (!def.mesh_name.empty())
    //     p.mesh_ref = load_mesh(def.mesh_name);
    // else
    //     p.mesh_ref = load_mesh("quad");

    instance->particle_count++;

    return p;
}

static void UpdateParticles()
{
    if (g_vfx.particle_count == 0)
        return;

    float dt = GetFrameTime();

    for (u32 i = 0; i < MAX_PARTICLES; ++i)
    {
        VfxParticle* p = static_cast<VfxParticle*>(GetAt(g_vfx.particle_pool, i));
        if (p->emitter_index == INVALID_INDEX)
            continue;

        p->elapsed += dt;

        if (p->elapsed >= p->lifetime)
        {
            p->emitter_index = INVALID_INDEX;
            Free(p);
            continue;
        }

        f32 t = p->elapsed / p->lifetime;
        f32 curve_t = EvaluateCurve(p->speed_curve, t);
        f32 current_speed = Mix(p->speed_start, p->speed_end, curve_t);

        Vec2 vel = Normalize(p->velocity) * current_speed;
        vel += p->gravity * dt;
        vel *= (1.0f - p->drag * dt);

        p->position += vel * dt;
        p->velocity = vel;
        p->rotation = Mix(p->rotation_start, p->rotation_end, EvaluateCurve(p->rotation_curve, t));
    }
}

static VfxEmitter* CreateEmitter(VfxInstance* instance, const VfxEmitterDef& def)
{
    if (g_vfx.emitter_count >= MAX_EMITTERS)
        return nullptr;

    VfxEmitter* e = (VfxEmitter*)Alloc(g_vfx.emitter_pool, sizeof(VfxEmitter));
    assert(e);

    e->def = &def;
    e->elapsed = 0.0f;
    e->duration = GetRandom(def.duration);
    e->accumulator = 0.0f;
    e->instance_index = GetIndex(instance);
    g_vfx.emitter_count++;

    return e;
}

static void DestroyEmitter(VfxEmitter* emitter)
{
    emitter->instance_index = INVALID_INDEX;
    Free(emitter);
}

static void UpdateEmitters()
{
    float dt = GetFrameTime();
    for (u32 i=0; i<MAX_EMITTERS; i++)
    {
        VfxEmitter* e = (VfxEmitter*)GetAt(g_vfx.emitter_pool, i);
        if (e->instance_index == INVALID_INDEX)
            continue;

        if (e->rate <= 0.0000001f)
            continue;

        // Check emitter lifetime
        if (e->elapsed >= e->duration)
        {
            DestroyEmitter(e);
            continue;
        }

        e->accumulator += dt;

        while (e->accumulator >= e->rate)
        {
            EmitParticle(e);
            e->accumulator -= e->rate;
        }
    }
}

void Stop(const VfxHandle& handle)
{
    VfxInstance* instance = GetInstance(handle);
    if (!instance)
        return;

    // Stop all the emitters but let existing particles live out their lifetime
    for (u32 i = 0; i < MAX_EMITTERS; ++i)
    {
        VfxEmitter* e = (VfxEmitter*)GetAt(g_vfx.emitter_pool, i);
        if (e->instance_index == GetIndex(instance))
            e->rate = 0.0f;
    }
}

void Destroy(VfxHandle& handle)
{
    VfxInstance* instance = GetInstance(handle);
    if (!instance)
        return;

    Stop(handle);

    // Free any particles associated with the instance
    for (u32 i = 0; i < MAX_PARTICLES; ++i)
    {
        VfxParticle* p = (VfxParticle*)GetAt(g_vfx.particle_pool, i);
        if (p->emitter_index == INVALID_INDEX)
            continue;

        VfxEmitter* e = (VfxEmitter*)GetAt(g_vfx.emitter_pool, p->emitter_index);
        if (e->instance_index == GetIndex(instance))
        {
            p->emitter_index = INVALID_INDEX;
            Free(p);
        }
    }

    // Free any emitters associated with the instance
    for (u32 i = 0; i < MAX_EMITTERS; ++i)
    {
        VfxEmitter* e = (VfxEmitter*)GetAt(g_vfx.emitter_pool, i);
        if (e->instance_index == GetIndex(instance))
        {
            e->instance_index = INVALID_INDEX;
            Free(e);
        }
    }
}

// @draw
void DrawVfx()
{
    if (g_vfx.particle_count == 0)
        return;

    UpdateEmitters();
    UpdateParticles();

    for (int i=0; i<MAX_PARTICLES; i++)
    {
        VfxParticle* p = static_cast<VfxParticle*>(GetAt(g_vfx.particle_pool, i));
        if (p->emitter_index == INVALID_INDEX)
            continue;

        float t = p->elapsed / p->lifetime;
        float size = Mix(p->size_start, p->size_end, EvaluateCurve(p->size_curve, t));
        Color col = Mix(p->color_start, p->color_end, EvaluateCurve(p->color_curve, t));

        BindTransform(p->position, p->rotation, {size, size});
        BindColor(col);
        BindMaterial(g_vfx.material);
        DrawMesh(g_vfx.meshes[VFX_MESH_SQUARE]);

#if 0
        // Create particle transform
        mat4 particle_transform = translate(mat4(1.0f), p->position);

        // Apply billboard mode
        if (p->billboard_mode == 1) // Camera billboard
        {
            // Extract camera forward/right/up from view matrix
            // For now, simplified billboard
            particle_transform = scale(particle_transform, Vec2(size, size));
        }
        else if (p->billboard_mode == 2) // Velocity billboard
        {
            // Align to velocity direction
            if (length(p->velocity) > 0.001f)
            {
                vec3 forward = normalize(p->velocity);
                vec3 up = vec3(0, 1, 0);
                vec3 right = cross(up, forward);
                up = cross(forward, right);
                mat4 rot_matrix = lookAt(vec3(0), forward, up);
                particle_transform = particle_transform * rot_matrix;
            }
            particle_transform = scale(particle_transform, vec3(size, size, size));
        }
        else // No billboard
        {
            particle_transform = particle_transform * mat4_cast(p->rotation);
            particle_transform = scale(particle_transform, vec3(size, size, size));
        }

        // Bind transform and Color for this particle
        BindTransform( particle_transform * world_transform);
        BindColor(col);

        // Render the particle mesh
        DrawMesh(p->mesh);
#endif
    }
}

VfxHandle Play(Vfx* vfx, const Vec2& position)
{
    VfxImpl* impl = static_cast<VfxImpl*>(vfx);
    assert(impl);

    if (g_vfx.emitter_count + impl->emitter_count > MAX_EMITTERS)
        return INVALID_HANDLE;

    VfxInstance* instance = CreateInstance(vfx, position);
    if (!instance)
        return INVALID_HANDLE;

    for (u32 i=0, c=impl->emitter_count; i<c; i++)
    {
        VfxEmitter* e = CreateEmitter(instance, impl->emitters[i]);
        assert(e);

        // Handle burst
        i32 burst_count = GetRandom(e->def->burst);
        for (i32 b = 0; b < burst_count; ++b)
            EmitParticle(e);
    }

    return GetHandle(instance);
}

bool IsPlaying(const VfxHandle& handle)
{
    return GetInstance(handle) != nullptr;
}

void ClearVfx()
{
    for (u32 i=0; i<MAX_PARTICLES; i++)
    {
        VfxParticle* p = (VfxParticle*)GetAt(g_vfx.particle_pool, i);
        if (p->emitter_index == INVALID_INDEX)
            continue;

        p->emitter_index = INVALID_INDEX;
        Free(p);
    }

    for (u32 i=0; i<MAX_EMITTERS; i++)
    {
        VfxEmitter* e = (VfxEmitter*)GetAt(g_vfx.emitter_pool, i);
        if (e->instance_index == INVALID_INDEX)
            continue;

        e->instance_index = INVALID_INDEX;
        Free(e);
    }

    for (u32 i=0; i<MAX_INSTANCES; i++)
    {
        VfxInstance* instance = (VfxInstance*)GetAt(g_vfx.instance_pool, i);
        if (!instance->vfx)
            continue;

        instance->vfx = nullptr;
        instance->version++;
        Free(instance);
    }

    g_vfx.particle_count = 0;
    g_vfx.emitter_count = 0;
    g_vfx.instance_count = 0;
}

void InitVfx()
{
    g_vfx.emitter_pool = CreatePoolAllocator(sizeof(VfxEmitter), MAX_EMITTERS);
    g_vfx.instance_pool = CreatePoolAllocator(sizeof(VfxInstance), MAX_INSTANCES);
    g_vfx.particle_pool = CreatePoolAllocator(sizeof(VfxParticle), MAX_PARTICLES);
    g_vfx.emitter_count = 0;
    g_vfx.particle_count = 0;
    g_vfx.instance_count = 0;

    for (u32 i=0; i<MAX_PARTICLES; i++)
    {
        VfxParticle* p = (VfxParticle*)GetAt(g_vfx.particle_pool, i);
        p->emitter_index = INVALID_INDEX;
    }

    for (u32 i=0; i<MAX_EMITTERS; i++)
    {
        VfxEmitter* e = (VfxEmitter*)GetAt(g_vfx.emitter_pool, i);
        e->instance_index = INVALID_INDEX;
    }

    g_vfx.material = CreateMaterial(ALLOCATOR_DEFAULT, g_core_assets.shaders.vfx);

    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
    AddQuad(builder, VEC2_UP, VEC2_RIGHT, 1, 1, {0,0});
    g_vfx.meshes[VFX_MESH_SQUARE] = CreateMesh(ALLOCATOR_DEFAULT, builder, GetName("vfx_square"));
    Free(builder);
}

void ShutdownVfx()
{
    Destroy(g_vfx.emitter_pool);
    Destroy(g_vfx.instance_pool);
    Destroy(g_vfx.particle_pool);

    g_vfx = {};
}