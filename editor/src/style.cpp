//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "style.h"

static Style g_styles[STYLE_ID_COUNT];

Style* g_style;

void InitStyles() {
    g_style = g_styles + STYLE_ID_DARK;

    // @dark
    g_style->background_color = Color24ToColor(0x383838);
    g_style->selection_color = Color24ToColor(0x3a79bb);
    g_style->button_color = Color24ToColor(0x585858);
    g_style->button_hover_color = Color24ToColor(0x676767);
    g_style->button_text_color = Color24ToColor(0xe3e3e3);
    g_style->button_checked_color = Color24ToColor(0x557496);
    g_style->button_checked_text_color = Color24ToColor(0xf0f0f0);
    g_style->button_disabled_color = Color24ToColor(0x2a2a2a);
    g_style->button_disabled_text_color = Color24ToColor(0x636363);
    g_style->workspace_color = Color24ToColor(0x464646);
    g_style->grid_color = Color24ToColor(0x686868);
    g_style->overlay_background_color = Color24ToColor(0x0e0e0e);
    g_style->overlay_text_color = Color24ToColor(0x979797);
    g_style->overlay_accent_text_color = Color24ToColor(0xd2d2d2);
    g_style->overlay_disabled_text_color = Color24ToColor(0x4a4a4a);
    g_style->overlay_icon_color = Color24ToColor(0x585858);
    g_style->overlay_content_color = Color24ToColor(0x2a2a2a);
    g_style->context_menu_separator_color = Color24ToColor(0x2a2a2a);
    g_style->context_menu_title_color = Color24ToColor(0x636363);
}
