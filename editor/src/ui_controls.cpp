//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

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

bool EditorButton(ElementId id, Mesh* icon, bool state, bool disabled) {
    bool was_pressed = false;
    BeginContainer({
        .width=STYLE_TOGGLE_BUTTON_HEIGHT,
        .height=STYLE_TOGGLE_BUTTON_HEIGHT,
        .id = id});

    bool hovered = IsHovered();
    was_pressed = WasPressed();

    BeginContainer({
        .padding=EdgeInsetsAll(STYLE_TOGGLE_BUTTON_PADDING),
        .color=STYLE_EDITOR_BUTTON_COLOR(state, hovered, disabled),
        .border={.radius=STYLE_TOGGLE_BUTTON_BORDER_RADIUS}});

    Image(icon, {
        .align=ALIGN_CENTER,
        .color=STYLE_EDITOR_BUTTON_TEXT_COLOR(state, disabled),
        .material = g_view.editor_mesh_material,
    });

    EndContainer();
    EndContainer();
    return was_pressed;
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
    Image(MESH_ICON_CLOSE);
    pressed = WasPressed();
    EndContainer();

    return pressed;
}