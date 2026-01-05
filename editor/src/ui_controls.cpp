//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

bool EditorToggleButton(ElementId id, Mesh* icon, bool state, bool disabled) {
    bool was_pressed = false;
    BeginContainer({
        .width=STYLE_TOGGLE_BUTTON_HEIGHT,
        .height=STYLE_TOGGLE_BUTTON_HEIGHT,
        .padding=EdgeInsetsAll(STYLE_TOGGLE_BUTTON_PADDING),
        .color=disabled
            ? STYLE_BUTTON_DISABLED_COLOR()
            : (state ? STYLE_BUTTON_CHECKED_COLOR() : STYLE_BUTTON_COLOR()),
        .border={.radius=STYLE_TOGGLE_BUTTON_BORDER_RADIUS},
        .id = id});
    was_pressed = WasPressed();
    Image(icon, {
        .align=ALIGN_CENTER,
        .color=disabled
            ? STYLE_BUTTON_DISABLED_TEXT_COLOR() :
            (state ? STYLE_BUTTON_CHECKED_TEXT_COLOR() : STYLE_BUTTON_TEXT_COLOR()),
        .material = g_view.editor_mesh_material,
    });
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