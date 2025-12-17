//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct CommandInput {
    const CommandHandler* commands = nullptr;
    const char* prefix = nullptr;
    const char* placeholder = nullptr;
    bool enabled;
    bool hide_empty;
    InputSet* input;
    Command command;
    bool pop_input;
    Text text;
};

static CommandInput g_command_input = {};

bool IsCommandInputActive() {
    return g_command_input.enabled;
}

void HandleCommand(const Command& command) {
    for (const CommandHandler* cmd = g_command_input.commands; cmd->name != nullptr; cmd++) {
        if (cmd->name == NAME_NONE || command.name == cmd->name || command.name == cmd->short_name) {
            cmd->handler(command);
            return;
        }
    }

    LogError("Unknown command: %s", command.name->value);
}

bool ParseCommand(const char* str, Command& command) {
    Tokenizer tk;
    Init(tk, str);

    if (!ExpectIdentifier(tk))
        return NAME_NONE;

    command.arg_count = 0;
    command.name = GetName(tk);

    Token token = {};
    while (ExpectToken(tk, &token))
        GetString(token, command.args[command.arg_count++], MAX_COMMAND_ARG_SIZE);

    if (command.arg_count <= 0 && tk.input[tk.position-1] != ' ')
        return false;

    return true;
}

constexpr int   COMMAND_FONT_SIZE = 24;
constexpr float COMMAND_HEIGHT = 48.0f;
constexpr float COMMAND_WIDTH = 600.0f;
constexpr float COMMAND_PADDING = 8.0f;

void UpdateCommandInput() {
    if (!g_command_input.enabled)
        return;

    bool commit = false;
    bool cancel = false;

    BeginCanvas({.id=CANVAS_ID_COMMAND});
    BeginContainer({
        .width=COMMAND_WIDTH,
        .align=ALIGN_BOTTOM_CENTER,
        .margin=EdgeInsetsBottom(160)});
    TextBox(g_command_input.text, {
        .height=COMMAND_HEIGHT,
        .padding=COMMAND_PADDING,
        .font = FONT_SEGUISB,
        .font_size = COMMAND_FONT_SIZE,
        .background_color = COLOR_UI_BACKGROUND,
        .border={.radius=10, .width=2, .color=COLOR_UI_BACKGROUND},
        .focus_border={.radius=10, .width=2, .color=STYLE_SELECTED_COLOR},
        .id = 1
    });

    commit = WasButtonPressed(KEY_ENTER);
    cancel = WasButtonPressed(KEY_ESCAPE);

    EndContainer();
    EndCanvas();

    if (commit) {
        ParseCommand(g_command_input.text.value, g_command_input.command);
        HandleCommand(g_command_input.command);
        EndCommandInput();
    } else if (cancel) {
        EndCommandInput();
    }
}

void BeginCommandInput(const CommandInputOptions& options) {
    g_command_input.enabled = true;
    g_command_input.commands = options.commands;
    g_command_input.prefix = options.prefix;
    g_command_input.placeholder = options.placeholder;
    g_command_input.hide_empty = options.hide_empty;

    if (!options.input) {
        PushInputSet(g_command_input.input);
        g_command_input.pop_input = true;
    } else if (options.input != GetInputSet()) {
        PushInputSet(options.input);
        g_command_input.pop_input = true;
    } else {
        g_command_input.pop_input = false;
    }

    SetFocus(CANVAS_ID_COMMAND, 1);
}

void EndCommandInput() {
    if (g_command_input.pop_input)
        PopInputSet();

    g_command_input.enabled = false;
    g_command_input.commands = nullptr;
    g_command_input.hide_empty = false;
    g_command_input.prefix = nullptr;
    g_command_input.placeholder = nullptr;
}

void InitCommandInput() {
    g_command_input = {};
    g_command_input.input = CreateInputSet(ALLOCATOR_DEFAULT);
    EnableButton(g_command_input.input, KEY_ESCAPE);
    EnableButton(g_command_input.input, KEY_ENTER);
}

void ShutdownCommandInput() {
    Free(g_command_input.input);
    g_command_input = {};
}
