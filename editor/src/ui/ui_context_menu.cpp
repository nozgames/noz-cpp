//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

constexpr ElementId CONTEXT_MENU_ID_CLOSE = ELEMENT_ID_MIN + 0;
constexpr ElementId CONTEXT_MENU_ID_ITEMS = ELEMENT_ID_MIN + 1;
constexpr ElementId CONTEXT_MENU_ID_MENU = CONTEXT_MENU_ID_ITEMS + CONTEXT_MENU_MAX_ITEMS;
constexpr int CONTEXT_MENU_SUBMENU_DEPTH = 8;

struct ContextMenu {
    bool visible;
    CanvasId last_canvas_id;
    ElementId last_element_id;
    Vec2 position;
    Vec2 world_position;
    ContextMenuConfig config;
    int open_submenu[CONTEXT_MENU_SUBMENU_DEPTH];
};

static ContextMenu g_context_menu = {};

static void ContextMenuSeparator() {
    BeginColumn({.height=STYLE_CONTEXT_MENU_SEPARATOR_SPACING});
    Expanded();
    Container({.height=STYLE_CONTEXT_MENU_SEPARATOR_HEIGHT, .color=STYLE_CONTEXT_MENU_SEPARATOR_COLOR()});
    Expanded();
    EndColumn();
}

static bool HasChildren(const ContextMenuConfig& config, int index) {
    if (index < 0 || index >= config.item_count - 1) return false;
    return config.items[index + 1].level > config.items[index].level;
}

static float ContextMenu(const ContextMenuConfig& config, int level, int parent_index, Vec2 menu_pos, bool* close, bool* item_pressed) {
    // Find items at this level that are children of parent_index
    int start_index = (level == 0) ? 0 : parent_index + 1;
    int parent_level = (level == 0) ? -1 : config.items[parent_index].level;

    ElementId menu_id = static_cast<ElementId>(CONTEXT_MENU_ID_MENU + level);
    BeginContainer({
        .min_width=STYLE_CONTEXT_MENU_MIN_WIDTH,
        .align=ALIGN_TOP_LEFT,
        .margin=EdgeInsetsTopLeft(menu_pos.y, menu_pos.x),
        .padding=EdgeInsetsAll(STYLE_OVERLAY_PADDING),
        .color=STYLE_OVERLAY_BACKGROUND_COLOR(),
        .border={.radius=STYLE_OVERLAY_BORDER_RADIUS},
        .id=menu_id
    });

    BeginColumn();

    if (level == 0 && config.title) {
        Label(config.title, {
            .font=FONT_SEGUISB,
            .font_size=STYLE_CONTEXT_MENU_TEXT_SIZE,
            .color=STYLE_CONTEXT_MENU_TITLE_COLOR()
        });
        ContextMenuSeparator();
    }

    float item_y = menu_pos.y + STYLE_OVERLAY_PADDING;
    if (level == 0 && config.title) {
        item_y += STYLE_CONTEXT_MENU_TEXT_SIZE + STYLE_CONTEXT_MENU_SEPARATOR_SPACING;
    }

    for (int index = start_index; index < config.item_count; index++) {
        const ContextMenuItem& item = config.items[index];

        if (item.level <= parent_level) break;
        if (item.level != parent_level + 1) continue;
        if (item.label == nullptr) {
            ContextMenuSeparator();
            continue;
        }

        bool has_children = HasChildren(config, index);
        bool is_submenu_open = (level < CONTEXT_MENU_SUBMENU_DEPTH) && (g_context_menu.open_submenu[level] == index);

        BeginContainer({.height=STYLE_CONTEXT_MENU_ITEM_HEIGHT, .id=static_cast<ElementId>(CONTEXT_MENU_ID_ITEMS + index)});

        bool hovered = IsHovered() && item.enabled;
        if (hovered || is_submenu_open) {
            Container({.margin=EdgeInsetsSymmetric(0, -4), .color=STYLE_SELECTION_COLOR()});
        }

        // Item label
        Label(item.label, {
            .font=FONT_SEGUISB,
            .font_size=STYLE_CONTEXT_MENU_TEXT_SIZE,
            .color=!item.enabled ?
                STYLE_OVERLAY_DISABLED_TEXT_COLOR() :
                (hovered || is_submenu_open)
                    ? STYLE_OVERLAY_ACCENT_TEXT_COLOR()
                    : STYLE_OVERLAY_TEXT_COLOR(),
            .align=ALIGN_CENTER_LEFT
        });

        // Submenu arrow indicator
        if (has_children) {
            BeginContainer({.width=STYLE_CONTEXT_MENU_TEXT_SIZE, .height=STYLE_CONTEXT_MENU_TEXT_SIZE, .align=ALIGN_CENTER_RIGHT, .padding=EdgeInsetsAll(2)});
            Image(MESH_ICON_SUBMENU, {
                .color=!item.enabled ?
                    STYLE_OVERLAY_DISABLED_TEXT_COLOR() :
                    (hovered || is_submenu_open)
                        ? STYLE_OVERLAY_ACCENT_TEXT_COLOR()
                        : STYLE_OVERLAY_TEXT_COLOR()
            });
            EndContainer();
        }

        if (WasPressed() && item.enabled) {
            *item_pressed = true;
            if (has_children) {
                // Toggle submenu
                if (is_submenu_open)
                    g_context_menu.open_submenu[level] = -1;
                else
                    g_context_menu.open_submenu[level] = index;

                // Close any deeper submenus
                for (int l = level + 1; l < CONTEXT_MENU_SUBMENU_DEPTH; l++)
                    g_context_menu.open_submenu[l] = -1;

            } else {
                if (item.action)
                    item.action();
                *close = true;
            }
        }

        // Open submenu on hover (after a parent is clicked open)
        if (hovered && has_children && item.enabled) {
            if (g_context_menu.open_submenu[level] >= 0 && g_context_menu.open_submenu[level] != index) {
                g_context_menu.open_submenu[level] = index;
                for (int l = level + 1; l < CONTEXT_MENU_SUBMENU_DEPTH; l++)
                    g_context_menu.open_submenu[l] = -1;
            }
        }

        EndContainer();

        item_y += STYLE_CONTEXT_MENU_ITEM_HEIGHT;
    }

    EndColumn();

    EndContainer();

    // Return actual width from last frame (falls back to min width on first frame)
    noz::Rect menu_rect = GetElementRect(menu_id);
    float actual_width = menu_rect.width > 0 ? menu_rect.width : (STYLE_CONTEXT_MENU_MIN_WIDTH + STYLE_OVERLAY_PADDING * 2);
    return actual_width;
}

bool UpdateContextMenu () {
    if (!g_context_menu.visible) return false;

    const ContextMenuConfig& config = g_context_menu.config;
    bool close = false;
    bool item_pressed = false;
    bool background_pressed = false;

    BeginCanvas({.id=CANVAS_ID_HELP});
    BeginContainer({.id=CONTEXT_MENU_ID_CLOSE});
    if (WasPressed()) background_pressed = true;
    EndContainer();

    // Render root menu
    float menu_width = ContextMenu(config, 0, -1, g_context_menu.position, &close, &item_pressed);

    // Track menu positions for each level
    Vec2 menu_positions[CONTEXT_MENU_SUBMENU_DEPTH + 1];
    menu_positions[0] = g_context_menu.position;

    // Render open submenus
    for (int level = 0; level < CONTEXT_MENU_SUBMENU_DEPTH; level++) {
        int parent_index = g_context_menu.open_submenu[level];
        if (parent_index < 0) break;

        // Get the parent menu's position
        Vec2 parent_menu_pos = menu_positions[level];

        // Calculate Y position of parent item within its menu
        float item_y = parent_menu_pos.y + STYLE_OVERLAY_PADDING;

        // Add title height only for root menu
        if (level == 0 && config.title) {
            item_y += STYLE_CONTEXT_MENU_TEXT_SIZE + STYLE_CONTEXT_MENU_SEPARATOR_SPACING;
        }

        // Count items before parent to get Y offset within parent menu
        int search_parent = (level == 0) ? -1 : g_context_menu.open_submenu[level - 1];
        int search_start = (level == 0) ? 0 : search_parent + 1;
        int search_parent_level = (level == 0) ? -1 : config.items[search_parent].level;

        for (int i = search_start; i < parent_index; i++) {
            const ContextMenuItem& item = config.items[i];
            // Stop if we've gone past siblings
            if (item.level <= search_parent_level) break;
            // Only count direct children (siblings of parent)
            if (item.level == search_parent_level + 1) {
                if (item.label == nullptr) {
                    item_y += STYLE_CONTEXT_MENU_SEPARATOR_SPACING;
                } else {
                    item_y += STYLE_CONTEXT_MENU_ITEM_HEIGHT;
                }
            }
        }

        // Position submenu to the right of parent menu
        Vec2 submenu_pos;
        submenu_pos.x = parent_menu_pos.x + menu_width + 2;
        submenu_pos.y = item_y;

        menu_positions[level + 1] = submenu_pos;
        menu_width = ContextMenu(config, level + 1, parent_index, submenu_pos, &close, &item_pressed);
    }

    // Close from background click only if no menu item was pressed
    if (background_pressed && !item_pressed) {
        close = true;
    }

    EndCanvas();

    if (WasButtonPressed(KEY_ESCAPE)) {
        // Close deepest open submenu first, or close menu if none open
        bool closed_submenu = false;
        for (int level = CONTEXT_MENU_SUBMENU_DEPTH - 1; level >= 0; level--) {
            if (g_context_menu.open_submenu[level] >= 0) {
                g_context_menu.open_submenu[level] = -1;
                closed_submenu = true;
                break;
            }
        }
        if (!closed_submenu) {
            close = true;
        }
    }

    if (close) {
        g_context_menu.visible = false;
        if (GetFocusedCanvasId() == CANVAS_ID_HELP)
            SetFocus(g_context_menu.last_canvas_id, g_context_menu.last_element_id);
    }

    return g_context_menu.visible;
}

void OpenContextMenuAtMouse(const ContextMenuConfig& config) {
    g_context_menu.position = ScreenToUI(GetMousePosition());
    g_context_menu.world_position = ScreenToWorld(g_view.camera, GetMousePosition());
    g_context_menu.visible = true;
    g_context_menu.last_canvas_id = GetFocusedCanvasId();
    g_context_menu.last_element_id = GetFocusedElementId();
    g_context_menu.config = config;
    for (int i = 0; i < CONTEXT_MENU_SUBMENU_DEPTH; i++) {
        g_context_menu.open_submenu[i] = -1;
    }
    SetFocus(CANVAS_ID_HELP, CONTEXT_MENU_ID_CLOSE);
}

Vec2 GetContextMenuWorldPosition() {
    return g_context_menu.world_position;
}
