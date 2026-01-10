//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {

    constexpr Color BOX_SELECT_COLOR = Color {0.2f, 0.6f, 1.0f, 0.025f};
    constexpr Color BOX_SELECT_OUTLINE_COLOR = Color {0.2f, 0.6f, 1.0f, 0.2f};
    constexpr float BOX_SELECT_EDGE_WIDTH = 0.005f;

    struct BoxSelect {
        void (*callback)(const Bounds2& bounds);
        Bounds2 selection;
    };

    static BoxSelect g_box_select = {};

    static void EndBoxSelect() {
        g_box_select = {};
        EndTool();
    }

    static void CommitBoxSelect() {
        g_box_select.callback(g_box_select.selection);
        EndBoxSelect();
        ConsumeButton(MOUSE_LEFT);
    }

    static void UpdateBoxSelect() {
        if (!g_workspace.drag) {
            CommitBoxSelect();
            return;
        }

        g_box_select.selection.min = Min(g_workspace.drag_world_position, g_workspace.mouse_world_position);
        g_box_select.selection.max = Max(g_workspace.drag_world_position, g_workspace.mouse_world_position);
    }

    static void DrawBoxSelect() {
        Vec2 center = GetCenter(g_box_select.selection);
        Vec2 size = GetSize(g_box_select.selection);

        // center
        BindColor(BOX_SELECT_COLOR);
        BindMaterial(g_workspace.vertex_material);
        BindTransform(TRS(center, 0, size * 0.5f));
        DrawMesh(g_workspace.edge_mesh);

        // outline
        float edge_width = g_workspace.zoom_ref_scale * BOX_SELECT_EDGE_WIDTH;
        BindColor(Color{0.2f, 0.6f, 1.0f, 0.8f});
        BindTransform(TRS(Vec2{center.x, g_box_select.selection.max.y}, 0, Vec2{size.x * 0.5f + edge_width, edge_width}));
        DrawMesh(g_workspace.edge_mesh);
        BindTransform(TRS(Vec2{center.x, g_box_select.selection.min.y}, 0, Vec2{size.x * 0.5f + edge_width, edge_width}));
        DrawMesh(g_workspace.edge_mesh);
        BindTransform(TRS(Vec2{g_box_select.selection.min.x, center.y}, 0, Vec2{edge_width, size.y * 0.5f + edge_width}));
        DrawMesh(g_workspace.edge_mesh);
        BindTransform(TRS(Vec2{g_box_select.selection.max.x, center.y}, 0, Vec2{edge_width, size.y * 0.5f + edge_width}));
        DrawMesh(g_workspace.edge_mesh);
    }

    void BeginBoxSelect(void (*callback)(const Bounds2& bounds)) {
        assert(callback);

        static ToolVtable box_select_vtable = {
            .update = UpdateBoxSelect,
            .draw = DrawBoxSelect,
        };

        BeginTool({
            .type = TOOL_TYPE_BOX_SELECT,
            .vtable = box_select_vtable,
            .input = g_workspace.input_tool,
            .inherit_input = true
        });

        g_box_select = {};
        g_box_select.callback = callback;
    }
}
