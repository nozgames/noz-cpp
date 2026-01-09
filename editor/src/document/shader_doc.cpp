//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {

    static void DrawShaderDocument(Document* doc) {
        BindMaterial(g_view.editor_mesh_material);
        BindColor(COLOR_WHITE);
        DrawMesh(MESH_ASSET_ICON_SHADER, Translate(doc->position));
    }

    static void InitShaderDocument(ShaderDocument* doc) {
        doc->vtable = {
            .draw = DrawShaderDocument,
        };
    }

    static void InitShaderDocument(Document* doc) {
        assert(doc);
        assert(doc->type == ASSET_TYPE_SHADER);
        InitShaderDocument(static_cast<ShaderDocument*>(doc));
    }

    void InitShaderDocumentDef() {
        InitDocumentDef({
            .type = ASSET_TYPE_SHADER,
            .size = sizeof(ShaderDocument),
            .ext=".glsl",
            .init_func = InitShaderDocument,
        });
    }
}
