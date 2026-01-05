//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

constexpr ElementId CONTEXT_MENU_ID_CLOSE = ELEMENT_ID_MIN + 0;
constexpr ElementId CONTEXT_MENU_ID_ITEMS = ELEMENT_ID_MIN + 1;

struct ContextMenu {
    bool visible;
    CanvasId last_canvas_id;
    ElementId last_element_id;
    Vec2 position;
    ContextMenuConfig config;
};

static ContextMenu g_context_menu = {};

static void ContextMenuSeparator() {
    BeginColumn({.height=STYLE_CONTEXT_MENU_SEPARATOR_SPACING});
    Expanded();
    Container({.height=STYLE_CONTEXT_MENU_SEPARATOR_HEIGHT, .color=STYLE_CONTEXT_MENU_SEPARATOR_COLOR()});
    Expanded();
    EndColumn();
}

bool UpdateContextMenu () {
    if (!g_context_menu.visible) return false;

    const ContextMenuConfig& config = g_context_menu.config;
    bool close = false;

    BeginCanvas({.id=CANVAS_ID_HELP});
    BeginContainer({.id=CONTEXT_MENU_ID_CLOSE});
    if (WasPressed()) close = true;
    EndContainer();

    BeginContainer({
        .min_width=STYLE_CONTEXT_MENU_MIN_WIDTH,
        .align=ALIGN_TOP_LEFT,
        .margin=EdgeInsetsTopLeft(g_context_menu.position.y, g_context_menu.position.x),
        .padding=EdgeInsetsAll(STYLE_OVERLAY_PADDING),
        .color=STYLE_OVERLAY_BACKGROUND_COLOR(),
        .border={.radius=STYLE_OVERLAY_BORDER_RADIUS}
    });

    BeginColumn();

    if (config.title) {
        Label(config.title, {
            .font=FONT_SEGUISB,
            .font_size=STYLE_CONTEXT_MENU_TEXT_SIZE,
            .color=STYLE_CONTEXT_MENU_TITLE_COLOR()
        });
        ContextMenuSeparator();
    }

    const ContextMenuItem* item = config.items;
    for (int index=0; item->label != nullptr; item++, index++) {
        BeginContainer({.height=STYLE_CONTEXT_MENU_ITEM_HEIGHT, .id=static_cast<ElementId>(CONTEXT_MENU_ID_ITEMS+index)});

        if (IsHovered() && item->enabled) {
            Container({.margin=EdgeInsetsSymmetric(0, -4), .color=STYLE_SELECTION_COLOR()});
        }

        Label(item->label, {
            .font=FONT_SEGUISB,
            .font_size=STYLE_CONTEXT_MENU_TEXT_SIZE,
            .color=!item->enabled ?
                STYLE_OVERLAY_DISABLED_TEXT_COLOR() :
                IsHovered()
                    ? STYLE_OVERLAY_ACCENT_TEXT_COLOR()
                    : STYLE_OVERLAY_TEXT_COLOR(),
            .align=ALIGN_CENTER_LEFT
        });

        if (WasPressed() && item->enabled) {
            if (item->action)
                item->action();
            close = true;
        }

        EndContainer();
    }

    EndColumn();

    EndContainer();
    EndCanvas();

    if (WasButtonPressed(KEY_ESCAPE))
        close = true;

    if (close) {
        g_context_menu.visible = false;
        if (GetFocusedCanvasId() == CANVAS_ID_HELP)
            SetFocus(g_context_menu.last_canvas_id, g_context_menu.last_element_id);
    }

    return g_context_menu.visible;
}

void OpenContextMenuAtMouse(const ContextMenuConfig& config) {
    g_context_menu.position = ScreenToUI(GetMousePosition());
    g_context_menu.visible = true;
    g_context_menu.last_canvas_id = GetFocusedCanvasId();
    g_context_menu.last_element_id = GetFocusedElementId();
    g_context_menu.config = config;
    SetFocus(CANVAS_ID_HELP, CONTEXT_MENU_ID_CLOSE);
}
