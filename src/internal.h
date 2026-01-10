//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//
#pragma once

struct lua_State;

namespace noz {

    struct SamplerOptions {
        TextureFilter filter;
        TextureClamp clamp;
    };

    // Function to compare sampler options
    bool sampler_options_equals(SamplerOptions* a, SamplerOptions* b);

    typedef u32 ShaderFlags;
    constexpr ShaderFlags SHADER_FLAGS_NONE = 0;
    constexpr ShaderFlags SHADER_FLAGS_BLEND = 1 << 0;
    constexpr ShaderFlags SHADER_FLAGS_DEPTH = 1 << 1;
    constexpr ShaderFlags SHADER_FLAGS_DEPTH_LESS = 1 << 2;
    constexpr ShaderFlags SHADER_FLAGS_POSTPROCESS = 1 << 3;
    constexpr ShaderFlags SHADER_FLAGS_UI_COMPOSITE = 1 << 4;
    constexpr ShaderFlags SHADER_FLAGS_PREMULTIPLIED_ALPHA = 1 << 5;

    // @render
    void BeginUIPass();

    // @input
    void InitInput();
    void ShutdownInput();
    void UpdateInput();

    // @physics
    void InitPhysics();
    void ShutdownPhysics();
    void UpdatePhysics();

    // @animation
    struct AnimationBone {
        u8 index;
    };

    struct AnimationFrame {
        int transform0;
        int transform1;
        int event;
        float fraction0;
        float fraction1;
        float root_motion0;
        float root_motion1;
    };

    struct AnimationImpl : Animation {
        int bone_count;
        int transform_count;
        int transform_stride;
        int frame_count;
        int frame_rate;
        float frame_rate_inv;
        float duration;
        AnimationFlags flags;
        AnimationBone* bones;
        BoneTransform* transforms;
        AnimationFrame* frames;
    };

    // @skeleton
    struct SkeletonImpl : Skeleton {
        int bone_count;
        Bone* bones;
    };

    // @tween
    extern void InitTween();
    extern void ShutdownTween();


#if defined(NOZ_LUA)

    // @lua

    namespace lua {
        struct LuaAsset;

        enum LuaObjectType {
            LUA_OBJECT_TYPE_UNKNOWN,
            LUA_OBJECT_TYPE_MESH,
        };

        struct LuaObject {
            LuaObjectType type;
        };

        struct LuaAsset : LuaObject {
            Asset* asset;
        };

        LuaAsset* Wrap(lua_State* L, Asset* asset);

        struct ScriptImpl : Script  {
            ByteCode byte_code;
        };

        extern float GetNumberField(lua_State* L, int index, const char* field_name, float default_value);
        extern Align GetAlignField(lua_State* L, int index, const char* field_name, Align default_value);
        extern Color GetColorField(lua_State* L, int index, const char* field_name, const Color& default_value);
        extern u8 GetU8Field(lua_State* L, int index, const char* field_name, u8 default_value);
        extern i32 GetIntField(lua_State* L, int index, const char* field_name, int default_value);
        extern Asset* GetAssetField(lua_State* L, int index, const char* field_name, Asset* default_value);
        extern void InitLuaAsset(lua_State* L);
    }

#endif
}