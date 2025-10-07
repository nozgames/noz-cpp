//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

constexpr int MAX_ELEMENTS = 4096;

enum ElementType : u8 {
    ELEMENT_TYPE_UNKNOWN = 0,
    ELEMENT_TYPE_NONE,
    ELEMENT_TYPE_CONTAINER,
    ELEMENT_TYPE_BORDER,
    ELEMENT_TYPE_ROW,
    ELEMENT_TYPE_COLUMN,
    ELEMENT_TYPE_LABEL,
    ELEMENT_TYPE_IMAGE,
    ELEMENT_TYPE_STACK,
    ELEMENT_TYPE_COUNT
};

struct RowStyle {
    float spacing = 0.0f;
};

struct ColumnStyle {
    float spacing = 0.0f;
};

struct LabelStyle {
    Font* font = nullptr;
};

struct ImageStyle {
};

struct BorderStyle {
    float width = 1.0f;
    Color color = COLOR_WHITE;
};

struct RectangleStyle {
    Color color = COLOR_WHITE;
};

struct Element {
    ElementType type;
};

struct ContainerElement : Element {
};

struct BorderElement : Element {
    BorderStyle style;
};

struct RowElement : Element {
    RowStyle style;
};

struct ColumnElement : Element {
    ColumnStyle style;
};

struct LabelElement : Element {
    Font* font;
    float font_style;
};

struct StackElement : Element {

};

struct ImageElement : Element {
    Material* material = nullptr;
    Mesh* mesh = nullptr;
};

union FatElement {
    Element element;
    ImageElement image;
    LabelElement label;
    RowElement row;
    ColumnElement column;
    ContainerElement container;
    BorderElement border;
};

struct UI {
    Allocator* allocator;
    Element* elements[MAX_ELEMENTS];
    u32 element_count;
};

static UI g_ui = {};

static Element* CreateElement(ElementType type) {
    Element* element = static_cast<Element*>(Alloc(g_ui.allocator, sizeof(FatElement)));
    element->type = type;
    g_ui.elements[g_ui.element_count++] = element;
    return element;
}

void Row(const RowStyle& style, void (*children)() = nullptr) {
    RowElement* row = static_cast<RowElement*>(CreateElement(ELEMENT_TYPE_ROW));
    row->style = style;

    if (children)
        children();
}

void Column(const ColumnStyle& style, void (*children)() = nullptr) {
    ColumnElement* column = static_cast<ColumnElement*>(CreateElement(ELEMENT_TYPE_COLUMN));
    column->style = style;

    if (children)
        children();
}

void Stack(void (*children)() = nullptr) {
    //StackElement* stack = static_cast<StackElement*>(CreateElement(ELEMENT_TYPE_STACK));
    if (children)
        children();
}

void Border(void (*children)());
void Container(void (*children)());
void Label(const char* text, const LabelStyle& style = {});
void Image(Material* material, const ImageStyle& style = {});
void Image(Material* material, Mesh* mesh, const ImageStyle& style = {});
void Transform(void (*children)());
void Canvas(void (*children)());
void Rectangle(const RectangleStyle& stlye={});

void LayoutElements()
{

}

void InitUI() {
    g_ui.allocator = CreateArenaAllocator(sizeof(FatElement) * MAX_ELEMENTS, "UI");
}

void ShutdownUI() {
    Destroy(g_ui.allocator);
    g_ui = {};
}

void Test() {
    Canvas([] {
        Row({ .spacing=10 }, [] {
            Border([] {
                Rectangle({ .color = COLOR_RED });
                Label("test", { .font = nullptr });
            });
        });
        Row({}, [] {
        });
    });
}
