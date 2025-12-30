//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "vfx_internal.h"

constexpr u16 MAX_PARTICLES = 4096;
constexpr u16 MAX_EMITTERS = 2024;
constexpr u16 MAX_INSTANCES = 256;
constexpr u16 MAX_MESHES = 1;

enum VfxMesh {
    VFX_MESH_SQUARE
};

struct VfxParticle {
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
    VfxCurveType opacity_curve;
    float opacity_start;
    float opacity_end;
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

struct VfxEmitter {
    const VfxEmitterDef* def;
    float rate;
    float elapsed;
    float duration;
    float accumulator;
    bool active;
    u16 instance_index;
    int particle_count;
};

struct VfxInstance {
    Vfx* vfx;
    Mat3 transform;
    float depth;
    int emitter_count;
    bool loop;
    u32 version;
};

struct VfxSystem {
    Mesh* meshes[MAX_MESHES];
    Material* material;

    PoolAllocator* instance_pool;
    bool instance_valid[MAX_INSTANCES];

    PoolAllocator* particle_pool;
    bool particle_valid[MAX_PARTICLES];

    PoolAllocator* emitter_pool;
    bool emitter_valid[MAX_EMITTERS];
};

static VfxSystem g_vfx = {};

static VfxEmitter* CreateEmitter(VfxInstance* instance, const VfxEmitterDef& def);
static VfxParticle* EmitParticle(VfxEmitter* e);

static float EvaluateCurve(VfxCurveType curve, float t)
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

static u16 GetIndex(VfxInstance* i) { return (u16)GetIndex(g_vfx.instance_pool, i); }
static u16 GetIndex(VfxEmitter* e) { return (u16)GetIndex(g_vfx.emitter_pool, e); }
static u16 GetIndex(VfxParticle* p) { return (u16)GetIndex(g_vfx.particle_pool, p); }
static VfxParticle* GetParticle(int i) { return (VfxParticle*)GetAt(g_vfx.particle_pool, i); }
static VfxEmitter* GetEmitter(int i) { return (VfxEmitter*)GetAt(g_vfx.emitter_pool, i); }
static VfxHandle GetHandle(VfxInstance* instance) {
    return {
        GetIndex(instance),
        instance->version
    };
}
static VfxInstance* GetInstance(u32 index) { return static_cast<VfxInstance*>(GetAt(g_vfx.instance_pool, index)); }
static VfxInstance* GetInstance(const VfxHandle& handle)
{
    if (handle.version == INVALID_VFX_HANDLE.version && handle.id == INVALID_VFX_HANDLE.id)
        return nullptr;

    if (!g_vfx.instance_valid[handle.id])
        return nullptr;

    VfxInstance* i = GetInstance(handle.id);
    if (i->version != handle.version)
        return nullptr;

    return i;
}

static VfxInstance* GetInstance(VfxEmitter* emitter) { return GetInstance(emitter->instance_index); }

static void InstanceDestructor(void* ptr)
{
    VfxInstance* i = (VfxInstance*)ptr;
    i->version++;
    g_vfx.instance_valid[GetIndex(i)] = false;
}

static VfxInstance* CreateInstance(Vfx* vfx, const Mat3& transform, float depth) {
    if (IsFull(g_vfx.instance_pool))
        return nullptr;

    VfxInstance* instance = static_cast<VfxInstance*>(Alloc(g_vfx.instance_pool, sizeof(VfxInstance), InstanceDestructor));
    assert(instance);
    instance->transform = transform;
    instance->vfx = vfx;
    instance->depth = depth;
    instance->loop = static_cast<VfxImpl*>(vfx)->loop;

    g_vfx.instance_valid[GetIndex(instance)] = true;
    return instance;
}

static void ParticleDestructor(void *ptr) {
    VfxParticle* p = (VfxParticle*)ptr;
    VfxEmitter* e = GetEmitter(p->emitter_index);

    assert(e->particle_count > 0);
    e->particle_count--;
    g_vfx.particle_valid[GetIndex(p)] = false;
}

static VfxParticle* EmitParticle(VfxEmitter* e) {
    if (IsFull(g_vfx.emitter_pool))
        return nullptr;

    VfxParticle* p = (VfxParticle*)Alloc(g_vfx.particle_pool, sizeof(VfxParticle), ParticleDestructor);
    assert(p);

    VfxInstance* i = GetInstance(e);
    assert(i);

    float angle = Radians(GetRandom(e->def->angle));

    Vec2 dir = GetRandom(e->def->direction);
    if (dir.x == 0.0f && dir.y == 0.0f)
        dir = { Cos(angle), Sin(angle) };

    const VfxParticleDef& def = e->def->particle_def;
    p->position = TransformVector(i->transform, GetRandom(e->def->spawn));
    p->size_start = GetRandom(def.size.start);
    p->size_end = GetRandom(def.size.end);
    p->opacity_curve = def.opacity.type;
    p->opacity_start = GetRandom(def.opacity.start);
    p->opacity_end = GetRandom(def.opacity.end);
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
    p->rotation_end = p->rotation_start + Radians(GetRandom(def.rotation.end));
    p->rotation = p->rotation_start;
    p->emitter_index = GetIndex(e);

    p->mesh = def.mesh;
    if (p->mesh == nullptr)
        p->mesh = g_vfx.meshes[VFX_MESH_SQUARE];

    g_vfx.particle_valid[GetIndex(p)] = true;
    e->particle_count++;

    return p;
}

static void UpdateParticles() {
    if (IsEmpty(g_vfx.particle_pool))
        return;

    float dt = GetFrameTime();

    for (u32 i = 0; i < MAX_PARTICLES; ++i) {
        if (!g_vfx.particle_valid[i])
            continue;

        VfxParticle* p = GetParticle(i);
        p->elapsed += dt;

        if (p->elapsed >= p->lifetime) {
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

    for (int particle_index=0; particle_index<MAX_PARTICLES; particle_index++) {
        if (!g_vfx.particle_valid[particle_index])
            continue;

        VfxParticle* p = GetParticle(particle_index);
        assert(p);

        VfxEmitter* e = GetEmitter(p->emitter_index);
        assert(e);

        VfxInstance* i = GetInstance(e);
        assert(i);

        float t = p->elapsed / p->lifetime;
        float size = Mix(p->size_start, p->size_end, EvaluateCurve(p->size_curve, t));
        float opacity = Mix(p->opacity_start, p->opacity_end, EvaluateCurve(p->opacity_curve, t));
        Color col = Mix(p->color_start, p->color_end, EvaluateCurve(p->color_curve, t));

        BindDepth(i->depth);
        BindTransform(i->transform * TRS(p->position, Degrees(p->rotation), {size, size}));
        BindColor(SetAlpha(col, opacity));
        BindMaterial(g_vfx.material);
        DrawMesh(p->mesh);
    }
}

static void EmitterDestructor(void* ptr)
{
    VfxEmitter* e = static_cast<VfxEmitter*>(ptr);
    assert(e);
    assert(e->particle_count == 0);
    VfxInstance* i = GetInstance(e);
    assert(i);
    i->emitter_count--;
    g_vfx.emitter_valid[GetIndex(e)] = false;

    if (i->emitter_count == 0)
        Free(i);
}

static VfxEmitter* CreateEmitter(VfxInstance* instance, const VfxEmitterDef& def) {
    if (IsFull(g_vfx.emitter_pool))
        return nullptr;

    VfxEmitter* e = (VfxEmitter*)Alloc(g_vfx.emitter_pool, sizeof(VfxEmitter), EmitterDestructor);
    assert(e);

    e->def = &def;
    e->elapsed = 0.0f;
    e->duration = GetRandom(def.duration);
    e->accumulator = 0.0f;
    e->instance_index = GetIndex(instance);

    int rate = RandomInt(def.rate.min, def.rate.max);
    e->rate = rate > 0
        ? 1.0f / static_cast<float>(rate)
        : 0.0f;

    instance->emitter_count++;

    g_vfx.emitter_valid[GetIndex(e)] = true;

    return e;
}

static void UpdateEmitters() {
    if (IsEmpty(g_vfx.emitter_pool))
        return;

    float dt = GetFrameTime();
    for (u32 i=0; i<MAX_EMITTERS; i++) {
        if (!g_vfx.emitter_valid[i])
            continue;

        VfxEmitter* e = GetEmitter(i);;
        if (e->rate <= 0.0000001f || e->elapsed >= e->duration) {
            if (e->particle_count == 0)
                Free(e);
            continue;
        }

        e->accumulator += dt;

        while (e->accumulator >= e->rate) {
            EmitParticle(e);
            e->accumulator -= e->rate;
        }
    }
}

void Stop(const VfxHandle& handle) {
    VfxInstance* instance = GetInstance(handle);
    if (!instance)
        return;

    // Stop all the emitters but let existing particles live out their lifetime
    for (u32 i = 0; i < MAX_EMITTERS; ++i) {
        VfxEmitter* e = GetEmitter(i);
        if (e->instance_index == GetIndex(instance))
            e->rate = 0.0f;
    }
}

void Destroy(VfxHandle& handle) {
    VfxInstance* instance = GetInstance(handle);
    if (!instance)
        return;

    u16 index = (u16)handle.id;

    Stop(handle);

    // Free any particles associated with the instance
    for (u32 i = 0; i < MAX_PARTICLES; ++i) {
        if (!g_vfx.particle_valid[i])
            continue;

        VfxParticle* p = GetParticle(i);
        assert(p);

        VfxEmitter* e = GetEmitter(i);
        assert(e);

        if (e->instance_index != index)
            continue;

        Free(p);
    }

    // Free any emitters associated with the instance
    for (u32 i = 0; i < MAX_EMITTERS; ++i) {
        if (!g_vfx.emitter_valid[i])
            continue;

        VfxEmitter* e = GetEmitter(i);
        assert(e);

        if (e->instance_index != index)
            continue;

        Free(e);
    }
}

// @draw
void DrawVfx()
{
    UpdateEmitters();
    UpdateParticles();
}

VfxHandle Play(Vfx* vfx, const Mat3& transform, float depth) {
    VfxImpl* impl = static_cast<VfxImpl*>(vfx);
    assert(impl);

    VfxInstance* instance = CreateInstance(vfx, transform, depth);
    if (!instance)
        return INVALID_VFX_HANDLE;

    for (u32 i=0, c=impl->emitter_count; i<c; i++) {
        VfxEmitter* e = CreateEmitter(instance, impl->emitters[i]);
        if (!e) break;

        i32 burst_count = GetRandom(e->def->burst);
        for (i32 b = 0; b < burst_count; ++b)
            EmitParticle(e);
    }

    return GetHandle(instance);
}

VfxHandle Play(Vfx* vfx, const Vec2& position, float depth) {
    return Play(vfx, Translate(position), depth);
}

bool IsPlaying(const VfxHandle& handle)
{
    VfxInstance* instance = GetInstance(handle);
    return instance != nullptr;
}

void ClearVfx()
{
    for (u32 i=0; i<MAX_PARTICLES; i++)
        if (g_vfx.particle_valid[i])
            Free(GetAt(g_vfx.particle_pool, i));

    for (u32 i=0; i<MAX_EMITTERS; i++)
        if (g_vfx.emitter_valid[i])
            Free(GetAt(g_vfx.emitter_pool, i));

    for (u32 i=0; i<MAX_INSTANCES; i++)
        if (g_vfx.instance_valid[i])
            Free(GetAt(g_vfx.instance_pool, i));
}

void SetVfxPaletteTexture(Texture* texture) {
    if (!texture) return;
    SetTexture(g_vfx.material, texture);
}

void InitVfx() {
    g_vfx.emitter_pool = CreatePoolAllocator(sizeof(VfxEmitter), MAX_EMITTERS);
    g_vfx.instance_pool = CreatePoolAllocator(sizeof(VfxInstance), MAX_INSTANCES);
    g_vfx.particle_pool = CreatePoolAllocator(sizeof(VfxParticle), MAX_PARTICLES);

    for (u32 i=0; i<MAX_PARTICLES; i++)
        g_vfx.particle_valid[i] = false;

    for (u32 i=0; i<MAX_EMITTERS; i++)
        g_vfx.emitter_valid[i] = false;

    for (u32 i=0; i<MAX_INSTANCES; i++)
        g_vfx.instance_valid[i] = false;

    g_vfx.material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_VFX);
    if (TEXTURE)
        SetVfxPaletteTexture(TEXTURE[0]);

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

#if !defined(NOZ_BUILTIN_ASSETS)
void RestartVfx(Vfx* vfx)
{
    for (u32 i=0; i<MAX_EMITTERS; i++)
    {
        if (!g_vfx.emitter_valid[i])
            continue;

        VfxEmitter* e = GetEmitter(i);
        assert(e);

        assert(e->def);
        if (e->def->vfx != vfx)
            continue;

        // Make sure it is one of the old emitters
        VfxImpl* impl = static_cast<VfxImpl*>(vfx);
        bool match = false;
        for (u32 j=0; !match && j<impl->emitter_count; j++)
            match = e->def == &impl->emitters[j];

        if (!match)
            continue;

        VfxInstance* instance = GetInstance(e);
        if (!instance)
            continue;

        Mat3 transform = instance->transform;
        Stop(GetHandle(instance));

        Play(vfx, transform);
    }
}
#endif