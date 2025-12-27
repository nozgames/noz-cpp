//
//  NoZ Game Engine - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::lua {
    extern void InitLuaUI(lua_State*);
}

using namespace noz::lua;

// Field name constants
static const char* k_type = "type";
static const char* k_id = "id";
static const char* k_color = "color";
static const char* k_align = "align";
static const char* k_width = "width";
static const char* k_height = "height";
static const char* k_font = "font";
static const char* k_font_size = "font_size";
static const char* k_border_width = "border_width";
static const char* k_border_color = "border_color";
static const char* k_border_radius = "border_radius";
static const char* k_scale = "scale";
static const char* k_stretch = "stretch";

static int LuaBeginCanvas(lua_State* L) {
    CanvasStyle style = {};

    if (lua_istable(L, 1)) {
        style.type = static_cast<CanvasType>(GetU8Field(L, 1, k_type, style.type));
        style.color = GetColorField(L, 1, k_color, style.color);
        style.id = GetIntField(L, 1, k_id, style.id);
    }

    BeginCanvas(style);
    return 0;
}

static int LuaEndCanvas(lua_State* L) {
    (void)L;
    EndCanvas();
    return 0;
}

static int LuaBeginContainer(lua_State* L) {
    ContainerStyle style = {};

    if (lua_istable(L, 1)) {
        style.width = GetNumberField(L, 1, k_width, style.width);
        style.height = GetNumberField(L, 1, k_height, style.height);
        style.align = GetAlignField(L, 1, k_align, style.align);
        style.color = GetColorField(L, 1, k_color, style.color);
        style.id = GetU8Field(L, 1, k_id, style.id);
        style.border.width = GetNumberField(L, 1, k_border_width, style.border.width);
        style.border.color = GetColorField(L, 1, k_border_color, style.border.color);
        style.border.radius = GetNumberField(L, 1, k_border_radius, style.border.radius);
    }

    BeginContainer(style);
    return 0;
}

static int LuaBeginRow(lua_State* L) {
    (void)L;
    BeginRow();
    return 0;
}

static int LuaBeginColumn(lua_State* L) {
    (void)L;
    BeginColumn();
    return 0;
}

static int LuaEndRow(lua_State*) { EndRow(); return 0; }
static int LuaEndColumn(lua_State*) { EndColumn(); return 0; }
static int LuaEndContainer(lua_State*) { EndContainer(); return 0; }

static int LuaLabel(lua_State* L) {
    const char* text = lua_tostring(L, 1);
    LabelStyle style = {};
    if (lua_istable(L, 2)) {
        style.color = GetColorField(L, 2, k_color, style.color);
        style.align = GetAlignField(L, 2, k_align, style.align);
        style.font_size = GetIntField(L, 2, k_font_size, style.font_size);
        style.font = static_cast<Font*>(GetAssetField(L, 2, k_font, style.font));
    }
    Label(text, style);
    return 0;
}

static int LuaImage(lua_State* L) {
    if (!lua_isuserdata(L, 1))
        luaL_error(L, "Image: first argument must be a Texture or Mesh");

    LuaAsset* lua_asset = static_cast<LuaAsset*>(lua_touserdata(L, 1));
    if (!lua_asset || !lua_asset->asset)
        luaL_error(L, "Image: invalid asset");

    ImageStyle style = {};
    if (lua_istable(L, 2)) {
        style.stretch = static_cast<ImageStretch>(GetU8Field(L, 2, k_stretch, style.stretch));
        style.align = GetAlignField(L, 2, k_align, style.align);
        style.scale = GetNumberField(L, 2, k_scale, style.scale);
        style.color = GetColorField(L, 2, k_color, style.color);
    }

    Asset* asset = lua_asset->asset;
    if (asset->type == ASSET_TYPE_TEXTURE) {
        Image(static_cast<Texture*>(asset), style);
    } else if (asset->type == ASSET_TYPE_MESH) {
        Image(static_cast<Mesh*>(asset), style);
    } else {
        luaL_error(L, "Image: unsupported asset type");
    }

    return 0;
}

static int LuaRectangle(lua_State* L) {
    (void)L;
    return 0;
}

void noz::lua::InitLuaUI(lua_State* L) {
    luaL_Reg statics[] = {
        { "BeginCanvas", LuaBeginCanvas },
        { "EndCanvas", LuaEndCanvas },
        { "BeginContainer", LuaBeginContainer },
        { "EndContainer", LuaEndContainer },
        { "BeginRow", LuaBeginRow },
        { "EndRow", LuaEndRow },
        { "BeginColumn", LuaBeginColumn },
        { "EndColumn", LuaEndColumn },
        { "Label", LuaLabel },
        { "Image", LuaImage },
        { "Rectangle", LuaRectangle },
        { nullptr, nullptr }
    };

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    luaL_register(L, nullptr, statics);

    // Canvas types
    lua_pushinteger(L, CANVAS_TYPE_SCREEN);
    lua_setglobal(L, "CANVAS_TYPE_SCREEN");
    lua_pushinteger(L, CANVAS_TYPE_WORLD);
    lua_setglobal(L, "CANVAS_TYPE_WORLD");

    // Align constants
    lua_pushinteger(L, ALIGN_NONE);
    lua_setglobal(L, "ALIGN_NONE");
    lua_pushinteger(L, ALIGN_TOP);
    lua_setglobal(L, "ALIGN_TOP");
    lua_pushinteger(L, ALIGN_LEFT);
    lua_setglobal(L, "ALIGN_LEFT");
    lua_pushinteger(L, ALIGN_BOTTOM);
    lua_setglobal(L, "ALIGN_BOTTOM");
    lua_pushinteger(L, ALIGN_RIGHT);
    lua_setglobal(L, "ALIGN_RIGHT");
    lua_pushinteger(L, ALIGN_TOP_LEFT);
    lua_setglobal(L, "ALIGN_TOP_LEFT");
    lua_pushinteger(L, ALIGN_TOP_RIGHT);
    lua_setglobal(L, "ALIGN_TOP_RIGHT");
    lua_pushinteger(L, ALIGN_TOP_CENTER);
    lua_setglobal(L, "ALIGN_TOP_CENTER");
    lua_pushinteger(L, ALIGN_CENTER_LEFT);
    lua_setglobal(L, "ALIGN_CENTER_LEFT");
    lua_pushinteger(L, ALIGN_CENTER_RIGHT);
    lua_setglobal(L, "ALIGN_CENTER_RIGHT");
    lua_pushinteger(L, ALIGN_CENTER);
    lua_setglobal(L, "ALIGN_CENTER");
    lua_pushinteger(L, ALIGN_BOTTOM_LEFT);
    lua_setglobal(L, "ALIGN_BOTTOM_LEFT");
    lua_pushinteger(L, ALIGN_BOTTOM_RIGHT);
    lua_setglobal(L, "ALIGN_BOTTOM_RIGHT");
    lua_pushinteger(L, ALIGN_BOTTOM_CENTER);
    lua_setglobal(L, "ALIGN_BOTTOM_CENTER");

    // Image stretch constants
    lua_pushinteger(L, IMAGE_STRETCH_NONE);
    lua_setglobal(L, "IMAGE_STRETCH_NONE");
    lua_pushinteger(L, IMAGE_STRETCH_FILL);
    lua_setglobal(L, "IMAGE_STRETCH_FILL");
    lua_pushinteger(L, IMAGE_STRETCH_UNIFORM);
    lua_setglobal(L, "IMAGE_STRETCH_UNIFORM");
}
