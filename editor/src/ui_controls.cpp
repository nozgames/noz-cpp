//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
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
            (state ? STYLE_BUTTON_CHECKED_TEXT_COLOR() : STYLE_BUTTON_TEXT_COLOR()),});
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
