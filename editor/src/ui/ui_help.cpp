//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {

    constexpr int HELP_ID_CLOSE = ELEMENT_ID_MIN + 0;
    constexpr float HELP_KEY_SIZE = 24.0f;
    constexpr int   HELP_KEY_FONT_SIZE = 10;
    constexpr float HELP_KEY_BORDER_RADIUS = 8.0f;

    struct Help {
        bool visible;
    };

    static Help g_help = {};

    extern void WorkspaceHelp();
    extern void GeneralHelp();

    static void BeginHelpGroup(const char* title) {
        BeginContainer();
        BeginColumn({.spacing=4});
        BeginContainer({.height=STYLE_BUTTON_HEIGHT});
        Label(title, {.font=FONT_SEGUISB, .font_size=STYLE_OVERLAY_TEXT_SIZE, .color=STYLE_OVERLAY_ACCENT_TEXT_COLOR(), .align=ALIGN_CENTER_LEFT});
        EndContainer();
    }

    static void EndHelpGroup() {
        Spacer(8.0f);
        EndColumn();
        EndContainer();
    }

    static void HelpKey(InputCode key) {
        BeginContainer({
            .min_width=HELP_KEY_SIZE,
            .height=HELP_KEY_SIZE,
            .align=ALIGN_CENTER_LEFT,
            .padding=EdgeInsetsLeftRight(4),
            .color=STYLE_OVERLAY_ACCENT_TEXT_COLOR(),
            .border={.radius=HELP_KEY_BORDER_RADIUS}});
        Label(ToString(key), {
            .font=FONT_SEGUISB,
            .font_size=HELP_KEY_FONT_SIZE,
            .color=STYLE_OVERLAY_BACKGROUND_COLOR(),
            .align=ALIGN_CENTER
        });
        EndContainer();
    }

    static void Help(const Shortcut* shortcut, int index) {
        if (!shortcut->description)
            return;

        BeginRow({.color=index % 2 ? COLOR_TRANSPARENT : COLOR_WHITE_1PCT, .spacing=8});

        BeginContainer({.width=HELP_KEY_SIZE * 4.0f});
        BeginRow({.spacing=4});
        if (shortcut->ctrl) HelpKey(KEY_LEFT_CTRL);
        if (shortcut->alt) HelpKey(KEY_LEFT_ALT);
        if (shortcut->shift) HelpKey(KEY_LEFT_SHIFT);
        HelpKey(shortcut->button);
        EndRow();
        EndContainer();

        BeginContainer({.width=200.0f});
        Label(shortcut->description, {
            .font=FONT_SEGUISB,
            .font_size=STYLE_OVERLAY_TEXT_SIZE,
            .color=STYLE_OVERLAY_TEXT_COLOR(),
            .align=ALIGN_CENTER_LEFT});
        EndContainer();
        EndRow();
    }

    void HelpGroup(const char* title, const Shortcut* shortcuts) {
        BeginHelpGroup(title);
        for (int i = 0; shortcuts[i].button != INPUT_CODE_NONE; i++)
            Help(shortcuts + i, i);
        EndHelpGroup();
    }

    bool UpdateHelp () {
        if (!g_help.visible) return false;

        BeginCanvas({.id=CANVAS_ID_HELP});
        BeginContainer({
            .align=ALIGN_CENTER,
            .padding=EdgeInsetsAll(STYLE_OVERLAY_PADDING),
            .color=STYLE_OVERLAY_BACKGROUND_COLOR(),
            .border={.radius=20}
        });

        if (EditorCloseButton(HELP_ID_CLOSE) )
            g_help.visible = false;

        BeginRow();

        GeneralHelp();

        if (g_workspace.state == VIEW_STATE_DEFAULT) {
            WorkspaceHelp();
        } else if (g_workspace.state == VIEW_STATE_EDIT) {
            assert(GetActiveDocument());
            if (GetActiveDocument()->vtable.editor_help)
                GetActiveDocument()->vtable.editor_help();
        }

        EndRow();
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
}
