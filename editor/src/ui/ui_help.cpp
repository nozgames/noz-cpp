//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

struct Help {
    bool visible;
};

static Help g_help = {};

bool UpdateHelp () {
    if (!g_help.visible) return false;

    BeginCanvas({.id=CANVAS_ID_HELP});
    BeginContainer({.width=400, .height=400, .align=ALIGN_CENTER, .color=STYLE_OVERLAY_BACKGROUND_COLOR(), .border={.radius=20}});
    EndContainer();
    EndCanvas();

    if (WasButtonPressed(KEY_ESCAPE)) {
        g_help.visible = false;
        SetFocus(CANVAS_ID_OVERLAY, ELEMENT_ID_NONE);
    }

    return g_help.visible;
}

void ToggleHelp() {
    g_help.visible = !g_help.visible;

    SetFocus(CANVAS_ID_HELP, ELEMENT_ID_NONE);
}
