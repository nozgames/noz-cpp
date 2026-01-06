//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

constexpr int DEBUG_UI_ID_SCREEN = ELEMENT_ID_MIN + 0;

constexpr int   DEBUG_UI_INFO_TEXT_SIZE = 14;
constexpr Color DEBUG_UI_BACKGROUND_COLOR = Color24ToColor(0x0e0e0e);
constexpr Color DEBUG_UI_TITLE_COLOR = Color24ToColor(0x383838);
constexpr Color DEBUG_UI_TEXT_COLOR = Color24ToColor(0x979797);
constexpr float DEBUG_UI_BORDER_RADIUS = 16.0f;
constexpr float DEBUG_UI_PADDING = 12.0f;
constexpr float DEBUG_UI_PROPERTY_HEIGHT = 24.0f;
constexpr float DEBUG_UI_PROPERTY_NAME_WIDTH = 100.0f;
constexpr float DEBUG_UI_PROPERTY_VALUE_WIDTH = 60.0f;

constexpr float STAT_SPACING = 4.0f;

struct DebugUI {
    bool visible;
    CanvasId last_canvas_id;
    ElementId last_element_id;
    void (*game_section)();
};

static DebugUI g_debug_ui = {};

static void CloseDebugUI() {
    SetFocus(g_debug_ui.last_canvas_id, g_debug_ui.last_element_id);
    g_debug_ui.visible = false;
}

static void Section(const char* name, void (*section_callback)()) {
    BeginColumn();

    Label(name, {
        .font=FONT_DEFAULT,
        .font_size=DEBUG_UI_INFO_TEXT_SIZE,
        .color=DEBUG_UI_TITLE_COLOR,
        .align=ALIGN_CENTER_LEFT});

    BeginColumn();
    Spacer(2);
    Container({.height=2, .color=DEBUG_UI_TITLE_COLOR});
    Spacer(4);
    EndColumn();

    section_callback();

    EndColumn();
}

void DebugProperty(const char* name, const char* value) {
    BeginContainer({.height=DEBUG_UI_PROPERTY_HEIGHT});
    BeginRow();
    {
        // label
        BeginContainer({.width=DEBUG_UI_PROPERTY_NAME_WIDTH});
        Label(name, {.font=FONT_DEFAULT, .font_size=DEBUG_UI_INFO_TEXT_SIZE, .color=DEBUG_UI_TEXT_COLOR, .align=ALIGN_CENTER_LEFT});
        EndContainer();

        // value
        BeginContainer({.width=DEBUG_UI_PROPERTY_VALUE_WIDTH});
        Label(value, {.font=FONT_DEFAULT, .font_size=DEBUG_UI_INFO_TEXT_SIZE, .color=DEBUG_UI_TEXT_COLOR, .align=ALIGN_CENTER_LEFT});
        EndContainer();
    }
    EndRow();
    EndContainer();
}

void DebugProperty(const char* name, int value) {
    Text text;
    Format(text, "%d", value);
    DebugProperty(name, text.value);
}

static void UISection() {
    DebugProperty("canvas_id", g_debug_ui.last_canvas_id);
    DebugProperty("element_id", g_debug_ui.last_element_id);
}

void UpdateDebugUI() {
    if (!g_debug_ui.visible)
        return;

    if (WasButtonPressed(KEY_ESCAPE)) {
        CloseDebugUI();
        return;
    }

    BeginCanvas({.id=CANVAS_ID_DEBUG});
    BeginContainer({.id=DEBUG_UI_ID_SCREEN});

    BeginContainer({
        .align=ALIGN_CENTER,
        .padding=EdgeInsetsAll(DEBUG_UI_PADDING),
        .color=DEBUG_UI_BACKGROUND_COLOR,
        .border={.radius=DEBUG_UI_BORDER_RADIUS}
    });

    BeginRow({.spacing=16});
    Section("UI", UISection);

    if (g_debug_ui.game_section)
        Section("Game", g_debug_ui.game_section);

    EndRow();

    EndContainer();
    EndContainer();
    EndCanvas();
}

void ToggleDebugUI(void (*game_section)()) {
    g_debug_ui.visible = !g_debug_ui.visible;
    g_debug_ui.game_section = game_section;

    if (g_debug_ui.visible) {
        g_debug_ui.last_canvas_id = GetFocusedCanvasId();
        g_debug_ui.last_element_id = GetFocusedElementId();
        SetFocus(CANVAS_ID_DEBUG, DEBUG_UI_ID_SCREEN);
    } else {
        CloseDebugUI();
    }
}

void InitDebugUI() {
}

void ShutdownDebugUI() {
}

