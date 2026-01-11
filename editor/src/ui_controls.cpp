//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {

    inline Color STYLE_EDITOR_BUTTON_COLOR(bool state, bool hovered, bool disabled) {
        if (disabled) return STYLE_BUTTON_DISABLED_COLOR();
        if (state) return STYLE_BUTTON_CHECKED_COLOR();
        if (hovered) return STYLE_BUTTON_HOVER_COLOR();
        return STYLE_BUTTON_COLOR();
    }

    inline Color STYLE_EDITOR_BUTTON_TEXT_COLOR(bool state, bool disabled) {
        if (disabled) return STYLE_BUTTON_DISABLED_TEXT_COLOR();
        if (state) return STYLE_BUTTON_CHECKED_TEXT_COLOR();
        return STYLE_BUTTON_TEXT_COLOR();
    }

    bool EditorButton(const EditorButtonConfig& config) {
        bool was_pressed = false;
        BeginContainer({
            .width=config.width,
            .height=config.height,
            .id = config.id});

        bool hovered = !config.disabled && IsHovered();
        was_pressed = !config.disabled && WasPressed();

        BeginContainer({
            .padding=EdgeInsetsAll(STYLE_TOGGLE_BUTTON_PADDING),
            .color=STYLE_EDITOR_BUTTON_COLOR(config.checked, hovered, config.disabled),
            .border={.radius=STYLE_TOGGLE_BUTTON_BORDER_RADIUS}});

        if (config.content_func)
            config.content_func();

        if (config.icon) {
            Image(config.icon, {
                .align=ALIGN_CENTER,
                .color=STYLE_EDITOR_BUTTON_TEXT_COLOR(config.checked, config.disabled),
                .material = g_workspace.editor_mesh_material,
            });
        }

        EndContainer();

        if (config.popup_func && config.checked && !config.disabled) {
            if (!config.popup_func() )
                was_pressed = true;
        }

        EndContainer();
        return was_pressed;

    }

    bool EditorButton(ElementId id, Sprite* icon, bool state, bool disabled) {
        //return EditorButton({.id = id, .icon = icon, .checked = state, .disabled = disabled});
        return false;
    }

    bool EditorButton(ElementId id, Mesh* icon, bool state, bool disabled) {
        return EditorButton({
            .id = id,
            .icon = icon,
            .checked = state,
            .disabled = disabled
        });
    }

    void BeginOverlay(ElementId id, Align align) {
        BeginContainer({
            .align=align,
            .margin=EdgeInsetsAll(STYLE_WORKSPACE_PADDING),
            .padding=EdgeInsetsAll(STYLE_OVERLAY_PADDING),
            .color=STYLE_OVERLAY_BACKGROUND_COLOR(),
            .border{.radius=STYLE_OVERLAY_BORDER_RADIUS},
            .id = id});
    }

    void EndOverlay() {
        EndContainer();
    }

    bool EditorCloseButton(ElementId id) {
        bool pressed = false;
        BeginContainer({
            .width=STYLE_BUTTON_HEIGHT,
            .height=STYLE_BUTTON_HEIGHT,
            .align=ALIGN_TOP_RIGHT,
            .padding=EdgeInsetsAll(STYLE_BUTTON_PADDING),
            .id=id
        });
        Image(SPRITE_ICON_CLOSE);
        pressed = WasPressed();
        EndContainer();

        return pressed;
    }
}
