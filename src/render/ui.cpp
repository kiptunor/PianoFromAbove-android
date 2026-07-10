#include <filesystem>
#include <string>

#define IMGUI_DEFINE_MATH_OPERATORS

#include "../../assets/FA6FreeSolidFontData.h"
#include "../../assets/IconsFontAwesome6.h"
#include "../../assets/Metrophobic_Regular.h"
#include "../audio/playback.h"
#include "../config/channel_colors.h"
#include "../config/file_dialog_state.h"
#include "../config/midi_list.h"
#include "../config/soundfont_list.h"
#include "../file_helpers.h"
#include "../globals.h"
#include "../logger.h"
#include "../visualizer_handler.h"
#include "render.h"
#include "ui.h"

#include <SDL3/SDL_opengl.h> // brings in GL/gl.h
// or for modern OpenGL:
// #include <glad/gl.h> // if you use glad
#include <SDL3/SDL_opengles2.h> // for GLES2/3

// #include <DixelU/smic.h>
#include <backend_render/imgui_impl_sdl3.h>
#include <backend_render/imgui_impl_sdlrenderer3.h>
#include <file_dlg/ImGuiFileDialog.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_styles.h>










// #define PLATFORM_ANDROID



// - - - - [Internal variables] - - - -
bool                       file_info_window  = false;
bool                       log_buffer_window = false;
bool                       allow_audio_dev_ssave;
bool                       log_buffer_cleared = false;
static char                midi_search[128];
static char                sf_search[128];
static std::string         midi_search_text;
static std::string         sf_search_text;
static std::string         midi_file;
static bool                trigger_scroll_sf_list;
int                        selected_img_path_entry;
static int                 selected_soundfont      = 0;
int                        selected_lost_midi      = 0;
int                        selected_lost_soundfont = 0;
int                        selected_log_line       = 0;
ImFont                    *FONT_icon_set;
std::string                temp_widget_id;
std::ostringstream         file_info_fields;
std::vector<std::string>   current_soundfonts;
FileHelpers::FileInfo      current_file_info;
char                       file_name_buf[3100] = { 0 };
char                       file_size_buf[3100] = { 0 };
char                       last_mod_buf[3100]  = { 0 };
bool                       is_midi_info        = false;
FileHelpers::MidiParseInfo cached_midi_info;
bool                       show_midi_details = false;
// single_midi_info_collector *smic_ptr            = nullptr;
f32                        android_scale;
ImVec4                     text_color;
static float               font_scale;

// UI/Widget variables
bool                       UI::show_demo_window    = false;
bool                       UI::main_gui_window     = false;
int                        UI::live_note_speed     = 6000;
int                        UI::selected_midi_index = 0;
bool                       UI::velocity_filter     = true;
bool                       UI::loop_colors         = false;
bool                       UI::overlap_remover     = true;
bool                       UI::use_bg_image        = false;
bool                       UI::no_midi_duplicates;
bool                       UI::no_soundfont_duplicates;
bool                       UI::vsync;
bool                       UI::internal_logging;
bool                       UI::log_to_file;
int                        UI::fps;
bool                       UI::vertical_lines;
bool                       UI::draw_measure_lines;
bool                       UI::use_default_media_paths = true;
bool                       UI::background_image;
bool                       UI::show_full_path_lost_midis      = false;
bool                       UI::show_full_path_lost_soundfonts = false;
bool                       UI::ui_theming                     = false;
int                        UI::min_velocity;
int                        UI::max_velocity;
std::string                UI::last_midi_path;
std::string                UI::last_midi_file;
std::string                UI::last_sf_path;
ImVec4                     UI::clear_color;
UI::RGBAint                UI::liveColor;
ImVec4                     UI::ui_chcolors[16];
int                        UI::current_audio_dev;
std::vector<std::string>   UI::prev_images;
static int                 builtin_ui_theme_idx = 0;
// clang-format off
const char                 *builtin_ui_theme_names[] =
{
    "Silvana (Default)",
    "Moonlight",
    "Neon Abyss",
    "Crimson Azure",
    "Arctic Horizon",
    "Cyberpunk"
};
// clang-format on











void                       ApplyScaleToStyle(ImGuiStyle &style, float scale)
{
    style.WindowPadding            *= scale;
    style.FramePadding             *= scale;
    style.CellPadding              *= scale;
    style.ItemSpacing              *= scale;
    style.ItemInnerSpacing         *= scale;
    style.IndentSpacing            *= scale;
    style.ScrollbarSize            *= scale + 1.0f;
    style.ScrollbarPadding         *= scale;
    style.GrabMinSize              *= scale + 0.95f;

    style.WindowRounding           *= scale;
    style.ChildRounding            *= scale;
    style.FrameRounding            *= scale + 1.8f;
    style.PopupRounding            *= scale;
    style.GrabRounding             *= scale;
    style.ScrollbarRounding        *= scale + 2.0f;
    style.TabRounding              *= scale + 2.0f;

    style.WindowBorderSize         *= scale;
    style.ChildBorderSize          *= scale;
    style.PopupBorderSize          *= scale + 2.0f;
    style.FrameBorderSize          *= scale + 2.0f;
    style.TabBorderSize            *= scale + 2.0f;
    style.TabBarBorderSize         *= scale;

    style.WindowMinSize            *= scale;
    style.ColumnsMinSpacing        *= scale;
    style.WindowBorderHoverPadding  = std::max(6.0f, style.WindowBorderHoverPadding * scale);
    style.ColorMarkerSize          *= scale;
}


/*
    ██╗███╗   ██╗████████╗███████╗██████╗ ███╗   ██╗ █████╗ ██╗
    ██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗████╗  ██║██╔══██╗██║
    ██║██╔██╗ ██║   ██║   █████╗  ██████╔╝██╔██╗ ██║███████║██║
    ██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗██║╚██╗██║██╔══██║██║
    ██║██║ ╚████║   ██║   ███████╗██║  ██║██║ ╚████║██║  ██║███████╗
    ╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝

    ████████╗██╗  ██╗███████╗███╗   ███╗███████╗███████╗
    ╚══██╔══╝██║  ██║██╔════╝████╗ ████║██╔════╝██╔════╝
       ██║   ███████║█████╗  ██╔████╔██║█████╗  ███████╗
       ██║   ██╔══██║██╔══╝  ██║╚██╔╝██║██╔══╝  ╚════██║
       ██║   ██║  ██║███████╗██║ ╚═╝ ██║███████╗███████║
       ╚═╝   ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝╚══════╝╚══════╝

*/

void UI::SetMoonlightTheme()
{
    /*
    Theme: Moonlight
    Author: deathsu/madam-herta
    Original source: https://github.com/Madam-Herta/Moonlight
    */
    ImGuiStyle &style                            = ImGui::GetStyle();

    style.Alpha                                  = 1.0f;
    style.DisabledAlpha                          = 1.0f;
    style.WindowPadding                          = ImVec2(12.0f, 12.0f);
    style.WindowRounding                         = 11.5f;
    style.WindowBorderSize                       = 0.0f;
    style.WindowMinSize                          = ImVec2(20.0f, 20.0f);
    style.WindowTitleAlign                       = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition               = ImGuiDir_Right;
    style.ChildRounding                          = 0.0f;
    style.ChildBorderSize                        = 1.0f;
    style.PopupRounding                          = 0.0f;
    style.PopupBorderSize                        = 1.0f;
    style.FramePadding                           = ImVec2(20.0f, 3.400000095367432f);
    style.FrameRounding                          = 11.89999961853027f;
    style.FrameBorderSize                        = 0.0f;
    style.ItemSpacing                            = ImVec2(4.300000190734863f, 5.5f);
    style.ItemInnerSpacing                       = ImVec2(7.099999904632568f, 1.799999952316284f);
    style.CellPadding                            = ImVec2(12.10000038146973f, 9.199999809265137f);
    style.IndentSpacing                          = 0.0f;
    style.ColumnsMinSpacing                      = 4.900000095367432f;
    style.ScrollbarPadding                       = 2.0f;
    style.ScrollbarSize                          = 20.60000038146973f;
    style.GrabMinSize                            = 12.700000047683716f;
    style.GrabRounding                           = 8.0f; // Modified
    style.TabRounding                            = 8.89999961853027f;
    style.TabBorderSize                          = 0.0f;
    // style.TabMinWidthForCloseButton = 0.0f; // This seems to be deprecated in Imgui v1.91.9b
    style.ColorButtonPosition                    = ImGuiDir_Right;
    style.ButtonTextAlign                        = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign                    = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text]                  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.09250493347644806f, 0.100297249853611f, 0.1158798336982727f, 1.0f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.1120669096708298f, 0.1262156516313553f, 0.1545064449310303f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.9725490212440491f, 1.0f, 0.4980392158031464f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.971993625164032f, 1.0f, 0.4980392456054688f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.0f, 0.7953379154205322f, 0.4980392456054688f, 1.0f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.1821731775999069f, 0.1897992044687271f, 0.1974248886108398f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.1545050293207169f, 0.1545048952102661f, 0.1545064449310303f, 1.0f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.1414651423692703f, 0.1629818230867386f, 0.2060086131095886f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.1072951927781105f, 0.107295036315918f, 0.1072961091995239f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.1293079704046249f, 0.1479243338108063f, 0.1931330561637878f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.1459212601184845f, 0.1459220051765442f, 0.1459227204322815f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.9725490212440491f, 1.0f, 0.4980392158031464f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.999999463558197f, 1.0f, 0.9999899864196777f, 1.0f);
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabActive]             = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.1249424293637276f, 0.2735691666603088f, 0.5708154439926147f, 1.0f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.5215686559677124f, 0.6000000238418579f, 0.7019608020782471f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.03921568766236305f, 0.9803921580314636f, 0.9803921580314636f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.8841201663017273f, 0.7941429018974304f, 0.5615870356559753f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.9570815563201904f, 0.9570719599723816f, 0.9570761322975159f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.00f, 0.43f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavHighlight]          = ImVec4(0.266094446182251f, 0.2890366911888123f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);

#ifdef PLATFORM_ANDROID
    // font_scale = ImGui::GetFontSize() / UI_FONT_SIZE; // e.g., 55/22 = 2.5
    ApplyScaleToStyle(style, font_scale);
#endif
}

void UI::SetSilvanaTheme()
{
    ImGuiStyle &style                          = ImGui::GetStyle();

    style.Alpha                                = 1.0f;
    style.WindowRounding                       = 10.0f;
    style.ChildRounding                        = 6.0f;
    style.FrameRounding                        = 6.0f;
    style.PopupRounding                        = 10.0f;
    style.GrabRounding                         = 3.0f;
    style.ScrollbarRounding                    = 4.0f;
    style.TabRounding                          = 3.0f;
    style.WindowPadding                        = ImVec2(14, 14);
    style.FramePadding                         = ImVec2(12, 5);
    style.ItemSpacing                          = ImVec2(8, 7);
    style.ItemInnerSpacing                     = ImVec2(6, 4);
    style.IndentSpacing                        = 21.0f;
    style.ScrollbarSize                        = 28.0f;
    style.ScrollbarPadding                     = 6.0f;
    style.GrabMinSize                          = 12.0f;
    style.WindowBorderSize                     = 1.0f;
    style.ChildBorderSize                      = 1.0f;
    style.PopupBorderSize                      = 1.0f;
    style.FrameBorderSize                      = 1.0f;
    style.TabBorderSize                        = 1.0f;
    style.TabBarBorderSize                     = 2.0f;
    style.TabRounding                          = 8.0f;
    style.CellPadding                          = ImVec2(8, 12);
    style.TableAngledHeadersTextAlign          = ImVec2(0.50, 0.50);
    style.WindowTitleAlign                     = ImVec2(0.50, 0.50);
    style.WindowBorderHoverPadding             = 6.0f; // Imgui asserts because of this so no scale multiplication
    style.WindowMenuButtonPosition             = ImGuiDir_Right;
    style.ColorMarkerSize                      = 8.0f;


    ImVec4 *colors                             = style.Colors;
    colors[ImGuiCol_Text]                      = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]              = ImVec4(0.47f, 0.29f, 0.00f, 1.00f);
    colors[ImGuiCol_WindowBg]                  = ImVec4(0.08f, 0.08f, 0.08f, 0.92f);
    colors[ImGuiCol_ChildBg]                   = ImVec4(0.08f, 0.08f, 0.08f, 0.27f);
    colors[ImGuiCol_PopupBg]                   = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_Border]                    = ImVec4(0.51f, 0.28f, 0.13f, 1.00f);
    colors[ImGuiCol_BorderShadow]              = ImVec4(0.53f, 0.26f, 0.10f, 1.00f);
    colors[ImGuiCol_FrameBg]                   = ImVec4(0.15f, 0.09f, 0.06f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]            = ImVec4(0.63f, 0.28f, 0.10f, 1.00f);
    colors[ImGuiCol_FrameBgActive]             = ImVec4(0.74f, 0.29f, 0.02f, 1.00f);
    colors[ImGuiCol_TitleBg]                   = ImVec4(0.37f, 0.22f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive]             = ImVec4(0.80f, 0.34f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]          = ImVec4(0.31f, 0.18f, 0.08f, 1.00f);
    colors[ImGuiCol_MenuBarBg]                 = ImVec4(0.64f, 0.30f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]               = ImVec4(0.21f, 0.12f, 0.08f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]             = ImVec4(0.45f, 0.19f, 0.03f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]      = ImVec4(0.67f, 0.23f, 0.04f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]       = ImVec4(1.00f, 0.29f, 0.00f, 1.00f);
    colors[ImGuiCol_CheckMark]                 = ImVec4(1.00f, 0.34f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]                = ImVec4(1.00f, 0.34f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]          = ImVec4(1.00f, 0.49f, 0.22f, 1.00f);
    colors[ImGuiCol_Button]                    = ImVec4(0.38f, 0.20f, 0.09f, 1.00f);
    colors[ImGuiCol_ButtonHovered]             = ImVec4(0.55f, 0.20f, 0.04f, 1.00f);
    colors[ImGuiCol_ButtonActive]              = ImVec4(0.69f, 0.27f, 0.05f, 1.00f);
    colors[ImGuiCol_Header]                    = ImVec4(0.45f, 0.19f, 0.04f, 1.00f);
    colors[ImGuiCol_HeaderHovered]             = ImVec4(0.40f, 0.16f, 0.05f, 0.43f);
    colors[ImGuiCol_HeaderActive]              = ImVec4(0.82f, 0.31f, 0.00f, 1.00f);
    colors[ImGuiCol_Separator]                 = ImVec4(0.56f, 0.22f, 0.07f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]          = ImVec4(0.56f, 0.22f, 0.07f, 1.00f);
    colors[ImGuiCol_SeparatorActive]           = ImVec4(0.56f, 0.22f, 0.07f, 1.00f);
    colors[ImGuiCol_ResizeGrip]                = ImVec4(0.63f, 0.23f, 0.00f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]         = ImVec4(1.00f, 0.36f, 0.01f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]          = ImVec4(1.00f, 0.55f, 0.30f, 1.00f);
    colors[ImGuiCol_InputTextCursor]           = ImVec4(0.99f, 0.62f, 0.38f, 1.00f);
    colors[ImGuiCol_TabHovered]                = ImVec4(0.45f, 0.23f, 0.11f, 1.00f);
    colors[ImGuiCol_Tab]                       = ImVec4(0.34f, 0.22f, 0.13f, 1.00f);
    colors[ImGuiCol_TabSelected]               = ImVec4(0.78f, 0.32f, 0.07f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline]       = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
    colors[ImGuiCol_TabDimmed]                 = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected]         = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines]                 = ImVec4(1.00f, 0.53f, 0.19f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]          = ImVec4(1.00f, 0.40f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]             = ImVec4(0.88f, 0.48f, 0.17f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]      = ImVec4(1.00f, 0.57f, 0.28f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]             = ImVec4(0.29f, 0.13f, 0.07f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]         = ImVec4(0.75f, 0.27f, 0.00f, 1.00f);
    colors[ImGuiCol_TableBorderLight]          = ImVec4(0.59f, 0.25f, 0.08f, 1.00f);
    colors[ImGuiCol_TableRowBg]                = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TableRowBgAlt]             = ImVec4(0.20f, 0.15f, 0.13f, 1.00f);
    colors[ImGuiCol_TextLink]                  = ImVec4(1.00f, 0.72f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]            = ImVec4(0.91f, 0.34f, 0.00f, 0.71f);
    colors[ImGuiCol_TreeLines]                 = ImVec4(0.38f, 0.38f, 0.38f, 0.50f);
    colors[ImGuiCol_DragDropTarget]            = ImVec4(1.00f, 0.77f, 0.50f, 1.00f);
    colors[ImGuiCol_DragDropTargetBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_UnsavedMarker]             = ImVec4(1.00f, 0.57f, 0.00f, 1.00f);
    colors[ImGuiCol_NavCursor]                 = ImVec4(0.95f, 0.37f, 0.13f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]     = ImVec4(0.91f, 0.50f, 0.23f, 1.00f);
    colors[ImGuiCol_NavWindowingDimBg]         = ImVec4(0.55f, 0.30f, 0.18f, 0.50f);
    colors[ImGuiCol_ModalWindowDimBg]          = ImVec4(0.41f, 0.21f, 0.07f, 0.50f);

#ifdef PLATFORM_ANDROID
    // font_scale = ImGui::GetFontSize() / UI_FONT_SIZE; // e.g., 55/22 = 2.5
    ApplyScaleToStyle(style, font_scale);
#endif
}

// All these themes are made by SlothPlayer
void UI::SetArcticHorizonTheme()
{
    ImGuiStyle &style                          = ImGui::GetStyle();

    // Arctic Horizon theme - Cool, icy theme with blues and whites
    style.Alpha                                = 1.0f;
    style.WindowRounding                       = 6.0f;
    style.ChildRounding                        = 4.0f;
    style.FrameRounding                        = 3.0f;
    style.GrabRounding                         = 2.0f;
    style.ScrollbarRounding                    = 4.0f;
    style.ScrollbarPadding                     = 3.0f;
    style.TabRounding                          = 3.0f;
    style.WindowPadding                        = ImVec2(12, 12);
    style.FramePadding                         = ImVec2(8, 6);
    style.ItemSpacing                          = ImVec2(8, 6);
    style.ItemInnerSpacing                     = ImVec2(6, 4);
    style.IndentSpacing                        = 20.0f;
    style.ScrollbarSize                        = 25.0f;
    style.GrabMinSize                          = 12.0f;
    style.FrameBorderSize                      = 0.0f;
    style.TabBorderSize                        = 0.0f;
    style.PopupBorderSize                      = 1.0f;


    // Arctic Horizon color palette - Cool blues and icy whites
    ImVec4 *colors                             = style.Colors;
    colors[ImGuiCol_Text]                      = ImVec4(0.95f, 0.97f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]              = ImVec4(0.50f, 0.55f, 0.65f, 1.00f);
    colors[ImGuiCol_WindowBg]                  = ImVec4(0.08f, 0.12f, 0.18f, 1.00f);
    colors[ImGuiCol_ChildBg]                   = ImVec4(0.10f, 0.14f, 0.20f, 1.00f);
    colors[ImGuiCol_PopupBg]                   = ImVec4(0.08f, 0.12f, 0.18f, 1.00f);
    colors[ImGuiCol_Border]                    = ImVec4(0.20f, 0.30f, 0.45f, 1.00f);
    colors[ImGuiCol_BorderShadow]              = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                   = ImVec4(0.12f, 0.18f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]            = ImVec4(0.15f, 0.22f, 0.35f, 1.00f);
    colors[ImGuiCol_FrameBgActive]             = ImVec4(0.18f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_TitleBg]                   = ImVec4(0.10f, 0.16f, 0.24f, 1.00f);
    colors[ImGuiCol_TitleBgActive]             = ImVec4(0.15f, 0.22f, 0.35f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]          = ImVec4(0.08f, 0.12f, 0.18f, 1.00f);
    colors[ImGuiCol_MenuBarBg]                 = ImVec4(0.10f, 0.16f, 0.24f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]               = ImVec4(0.08f, 0.12f, 0.18f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]             = ImVec4(0.20f, 0.30f, 0.45f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]      = ImVec4(0.25f, 0.35f, 0.50f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]       = ImVec4(0.30f, 0.40f, 0.55f, 1.00f);
    colors[ImGuiCol_CheckMark]                 = ImVec4(0.20f, 0.80f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrab]                = ImVec4(0.20f, 0.80f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]          = ImVec4(0.30f, 0.85f, 0.95f, 1.00f);
    colors[ImGuiCol_Button]                    = ImVec4(0.15f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonHovered]             = ImVec4(0.20f, 0.25f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive]              = ImVec4(0.25f, 0.30f, 0.40f, 1.00f);
    colors[ImGuiCol_Header]                    = ImVec4(0.18f, 0.24f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderHovered]             = ImVec4(0.22f, 0.28f, 0.40f, 1.00f);
    colors[ImGuiCol_HeaderActive]              = ImVec4(0.26f, 0.32f, 0.45f, 1.00f);
    colors[ImGuiCol_Separator]                 = ImVec4(0.18f, 0.24f, 0.35f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]          = ImVec4(0.22f, 0.28f, 0.40f, 1.00f);
    colors[ImGuiCol_SeparatorActive]           = ImVec4(0.26f, 0.32f, 0.45f, 1.00f);
    colors[ImGuiCol_ResizeGrip]                = ImVec4(0.20f, 0.30f, 0.45f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]         = ImVec4(0.25f, 0.35f, 0.50f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]          = ImVec4(0.30f, 0.40f, 0.55f, 1.00f);
    colors[ImGuiCol_InputTextCursor]           = ImVec4(0.20f, 0.80f, 0.90f, 1.00f);
    colors[ImGuiCol_TabHovered]                = ImVec4(0.15f, 0.22f, 0.35f, 1.00f);
    colors[ImGuiCol_Tab]                       = ImVec4(0.12f, 0.18f, 0.28f, 1.00f);
    colors[ImGuiCol_TabSelected]               = ImVec4(0.18f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline]       = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
    colors[ImGuiCol_TabDimmed]                 = ImVec4(0.10f, 0.16f, 0.24f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected]         = ImVec4(0.14f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines]                 = ImVec4(0.00f, 0.83f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]          = ImVec4(0.00f, 1.00f, 0.55f, 1.00f);
    colors[ImGuiCol_PlotHistogram]             = ImVec4(0.00f, 0.54f, 0.82f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]      = ImVec4(0.00f, 0.83f, 1.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]             = ImVec4(0.18f, 0.24f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]         = ImVec4(0.20f, 0.80f, 0.90f, 1.00f);
    colors[ImGuiCol_TableBorderLight]          = ImVec4(0.25f, 0.30f, 0.40f, 1.00f);
    colors[ImGuiCol_TableRowBg]                = ImVec4(0.10f, 0.16f, 0.24f, 1.00f);
    colors[ImGuiCol_TableRowBgAlt]             = ImVec4(0.18f, 0.26f, 0.42f, 0.39f);
    colors[ImGuiCol_TextLink]                  = ImVec4(0.30f, 0.85f, 0.95f, 1.00f);
    colors[ImGuiCol_InputTextCursor]           = ImVec4(0.20f, 0.80f, 0.90f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]            = ImVec4(0.20f, 0.80f, 0.90f, 0.35f);
    colors[ImGuiCol_TreeLines]                 = ImVec4(0.38f, 0.38f, 0.38f, 0.50f);
    colors[ImGuiCol_DragDropTarget]            = ImVec4(0.18f, 0.28f, 0.46f, 1.00f);
    colors[ImGuiCol_DragDropTargetBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_UnsavedMarker]             = ImVec4(0.30f, 0.85f, 0.95f, 1.00f);
    colors[ImGuiCol_NavCursor]                 = ImVec4(0.14f, 0.50f, 0.87f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]     = ImVec4(0.07f, 0.48f, 0.73f, 0.83f);
    colors[ImGuiCol_NavWindowingDimBg]         = ImVec4(0.10f, 0.66f, 0.87f, 0.42f);
    colors[ImGuiCol_ModalWindowDimBg]          = ImVec4(0.02f, 0.05f, 0.12f, 0.70f);

#ifdef PLATFORM_ANDROID
    // font_scale = ImGui::GetFontSize() / UI_FONT_SIZE; // e.g., 55/22 = 2.5
    ApplyScaleToStyle(style, font_scale);
#endif
}

void UI::SetCrimsonAzureTheme()
{
    ImGuiStyle &style                      = ImGui::GetStyle();

    // -------------------------
    // Layout / Feel
    // -------------------------
    style.WindowRounding                   = 10.0f;
    style.ChildRounding                    = 8.0f;
    style.FrameRounding                    = 6.0f;
    style.PopupRounding                    = 8.0f;
    style.ScrollbarRounding                = 12.0f;
    style.GrabRounding                     = 6.0f;
    style.TabRounding                      = 7.0f;
    style.TabBorderSize                    = 1.0f;
    style.WindowPadding                    = ImVec2(12, 10);
    style.FramePadding                     = ImVec2(10, 6);
    style.ItemSpacing                      = ImVec2(10, 8);
    style.ItemInnerSpacing                 = ImVec2(8, 6);
    style.ScrollbarSize                    = 25.0f;
    style.ScrollbarPadding                 = 4.0f;
    style.GrabMinSize                      = 10.0f;
    style.FrameBorderSize                  = 1.0f;
    style.WindowBorderSize                 = 1.0f;
    style.PopupBorderSize                  = 1.0f;


    ImVec4 *colors                         = style.Colors;

    // -------------------------
    // Core Palette
    // -------------------------
    ImVec4  bg                             = ImVec4(0.06f, 0.07f, 0.10f, 1.00f); // deep dark
    ImVec4  bg2                            = ImVec4(0.09f, 0.10f, 0.14f, 1.00f); // panels
    ImVec4  blue                           = ImVec4(0.20f, 0.55f, 1.00f, 1.00f); // primary cyan-blue
    ImVec4  blueDeep                       = ImVec4(0.10f, 0.30f, 0.70f, 1.00f); // active blue
    ImVec4  red                            = ImVec4(1.00f, 0.25f, 0.35f, 1.00f); // neon red
    // ImVec4 redDeep   = ImVec4(0.70f, 0.10f, 0.15f, 1.00f); // active red
    ImVec4  text                           = ImVec4(0.92f, 0.94f, 0.98f, 1.00f);
    ImVec4  muted                          = ImVec4(0.60f, 0.62f, 0.70f, 1.00f);

    colors[ImGuiCol_Text]                  = text;
    colors[ImGuiCol_TextDisabled]          = muted;
    colors[ImGuiCol_WindowBg]              = bg;
    colors[ImGuiCol_ChildBg]               = bg2;
    colors[ImGuiCol_PopupBg]               = bg;
    colors[ImGuiCol_Border]                = ImVec4(0.18f, 0.20f, 0.26f, 0.55f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_FrameBg]               = bg2;
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.12f, 0.14f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.14f, 0.16f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBg]               = bg;
    colors[ImGuiCol_MenuBarBg]             = bg2;
    colors[ImGuiCol_TitleBgActive]         = bg2;
    colors[ImGuiCol_TitleBgCollapsed]      = bg;
    colors[ImGuiCol_TextLink]              = blueDeep;
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.200f, 0.549f, 1.000f, 0.387f);
    colors[ImGuiCol_Button]                = ImVec4(0.12f, 0.14f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = blue;
    colors[ImGuiCol_InputTextCursor]       = blue;
    colors[ImGuiCol_UnsavedMarker]         = blue;
    colors[ImGuiCol_ButtonActive]          = blueDeep;
    colors[ImGuiCol_CheckMark]             = blue;
    colors[ImGuiCol_SliderGrab]            = blue;
    colors[ImGuiCol_SliderGrabActive]      = red;
    colors[ImGuiCol_Separator]             = ImVec4(0.22f, 0.24f, 0.30f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]      = blue;
    colors[ImGuiCol_SeparatorActive]       = red;
    colors[ImGuiCol_Tab]                   = bg2;
    colors[ImGuiCol_TabHovered]            = blue;
    colors[ImGuiCol_TabActive]             = red;
    colors[ImGuiCol_TabUnfocused]          = bg;
    colors[ImGuiCol_TabUnfocusedActive]    = bg2;
    colors[ImGuiCol_Header]                = bg2;
    colors[ImGuiCol_HeaderHovered]         = blue;
    colors[ImGuiCol_HeaderActive]          = red;
    colors[ImGuiCol_ScrollbarBg]           = bg;
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.130f, 0.145f, 0.204f, 1.000f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = blue;
    colors[ImGuiCol_ScrollbarGrabActive]   = red;
    colors[ImGuiCol_PlotLines]             = blue;
    colors[ImGuiCol_PlotLinesHovered]      = red;
    colors[ImGuiCol_PlotHistogram]         = blue;
    colors[ImGuiCol_PlotHistogramHovered]  = red;
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_ResizeGripHovered]     = blue;
    colors[ImGuiCol_ResizeGripActive]      = red;
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0, 0, 0, 0.6f);
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.072f, 0.106f, 0.241f, 1.000f);
    colors[ImGuiCol_TableBorderStrong]     = blue;
    colors[ImGuiCol_TableBorderLight]      = blueDeep;
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.067f, 0.095f, 0.180f, 1.000f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.021f, 0.038f, 0.102f, 1.000f);
    colors[ImGuiCol_TreeLines]             = ImVec4(0.200f, 0.549f, 1.000f, 0.387f);
    colors[ImGuiCol_DragDropTarget]        = blue;
    colors[ImGuiCol_DragDropTargetBg]      = ImVec4(0.200f, 0.549f, 1.000f, 0.387f);
    colors[ImGuiCol_NavCursor]             = blue;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.200f, 0.549f, 1.000f, 0.387f);

#ifdef PLATFORM_ANDROID
    // font_scale = ImGui::GetFontSize() / UI_FONT_SIZE; // e.g., 55/22 = 2.5
    ApplyScaleToStyle(style, font_scale);
#endif
}

void UI::SetCyberpunkTheme()
{
    ImGuiStyle &style                     = ImGui::GetStyle();

    // -------------------------
    // Layout / Sizing
    // -------------------------
    style.WindowRounding                  = 10.0f;
    style.ChildRounding                   = 8.0f;
    style.FrameRounding                   = 6.0f;
    style.PopupRounding                   = 8.0f;
    style.ScrollbarRounding               = 12.0f;
    style.ScrollbarSize                   = 24.0f;
    style.ScrollbarPadding                = 6.0f;
    style.GrabRounding                    = 6.0f;
    style.TabRounding                     = 6.0f;
    style.PopupBorderSize                 = 1.0f;
    style.FrameBorderSize                 = 0.0f;

    style.WindowPadding                   = ImVec2(12, 10);
    style.FramePadding                    = ImVec2(10, 6);
    style.ItemSpacing                     = ImVec2(10, 8);
    style.ItemInnerSpacing                = ImVec2(8, 6);

    style.GrabMinSize                     = 10.0f;

    ImVec4 *colors                        = style.Colors;

    ImVec4  bg                            = ImVec4(0.06f, 0.07f, 0.10f, 1.00f);
    ImVec4  bg2                           = ImVec4(0.09f, 0.10f, 0.14f, 1.00f);
    ImVec4  accent                        = ImVec4(0.000f, 0.850f, 1.000f, 0.771f); // neon cyan
    ImVec4  accent2                       = ImVec4(0.85f, 0.25f, 1.00f, 1.00f);     // neon purple
    ImVec4  text                          = ImVec4(0.90f, 0.92f, 0.95f, 1.00f);
    ImVec4  mutedText                     = ImVec4(0.60f, 0.62f, 0.70f, 1.00f);

    colors[ImGuiCol_Text]                 = text;
    colors[ImGuiCol_InputTextCursor]      = accent;
    colors[ImGuiCol_TextDisabled]         = mutedText;
    colors[ImGuiCol_WindowBg]             = bg;
    colors[ImGuiCol_ChildBg]              = bg2;
    colors[ImGuiCol_PopupBg]              = bg;
    colors[ImGuiCol_Border]               = ImVec4(0.20f, 0.22f, 0.28f, 0.50f);
    colors[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_FrameBg]              = bg2;
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.12f, 0.14f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.14f, 0.16f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBg]              = bg;
    colors[ImGuiCol_TitleBgActive]        = bg2;
    colors[ImGuiCol_TitleBgCollapsed]     = bg;
    colors[ImGuiCol_Button]               = ImVec4(0.10f, 0.12f, 0.18f, 1.00f);
    colors[ImGuiCol_ButtonHovered]        = accent;
    colors[ImGuiCol_ButtonActive]         = accent2;
    colors[ImGuiCol_CheckMark]            = accent;
    colors[ImGuiCol_SliderGrab]           = accent;
    colors[ImGuiCol_SliderGrabActive]     = accent2;
    colors[ImGuiCol_Separator]            = ImVec4(0.20f, 0.22f, 0.30f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]     = accent;
    colors[ImGuiCol_SeparatorActive]      = accent2;
    colors[ImGuiCol_ResizeGrip]           = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_ResizeGripHovered]    = accent;
    colors[ImGuiCol_ResizeGripActive]     = accent2;
    colors[ImGuiCol_Tab]                  = bg2;
    colors[ImGuiCol_TabHovered]           = accent;
    colors[ImGuiCol_TabActive]            = accent2;
    colors[ImGuiCol_TabUnfocused]         = bg;
    colors[ImGuiCol_TabUnfocusedActive]   = bg2;
    colors[ImGuiCol_Header]               = bg2;
    colors[ImGuiCol_HeaderHovered]        = accent;
    colors[ImGuiCol_HeaderActive]         = accent2;
    colors[ImGuiCol_PlotLines]            = accent;
    colors[ImGuiCol_PlotLinesHovered]     = accent2;
    colors[ImGuiCol_PlotHistogram]        = accent;
    colors[ImGuiCol_PlotHistogramHovered] = accent2;
    colors[ImGuiCol_ScrollbarBg]          = bg;
    colors[ImGuiCol_ScrollbarGrab]        = bg2;
    colors[ImGuiCol_ScrollbarGrabHovered] = accent;
    colors[ImGuiCol_ScrollbarGrabActive]  = accent2;
    colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0, 0, 0, 0.6f);

#ifdef PLATFORM_ANDROID
    // font_scale = ImGui::GetFontSize() / UI_FONT_SIZE; // e.g., 55/22 = 2.5
    ApplyScaleToStyle(style, font_scale);
#endif
}

void UI::SetNeonAbyssTheme()
{
    ImGuiStyle &style                                = ImGui::GetStyle();

    style.Alpha                                      = 1.0f;
    style.WindowRounding                             = 3.0f;
    style.ChildRounding                              = 3.0f;
    style.FrameRounding                              = 3.0f;
    style.GrabRounding                               = 1.0f;
    style.ScrollbarRounding                          = 3.0f;
    style.GrabMinSize                                = 20.0f;
    style.WindowPadding                              = ImVec2(8.0f, 8.0f);
    style.FramePadding                               = ImVec2(6.0f, 4.0f);
    style.ItemSpacing                                = ImVec2(8.0f, 8.0f);
    style.ItemInnerSpacing                           = ImVec2(4.0f, 4.0f);
    style.IndentSpacing                              = 20.0f;
    style.ScrollbarSize                              = 20.0f;
    style.ScrollbarPadding                           = 4.0f;
    style.TabRounding                                = 3.0f;
    style.FrameBorderSize                            = 1.0f;
    style.TabBorderSize                              = 1.0f;
    style.PopupBorderSize                            = 1.0f;


    style.Colors[ImGuiCol_Text]                      = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]              = ImVec4(0.00f, 0.40f, 0.41f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]                  = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_ChildBg]                   = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg]                   = ImVec4(0.00f, 0.13f, 0.13f, 0.90f);
    style.Colors[ImGuiCol_Border]                    = ImVec4(0.00f, 1.00f, 1.00f, 0.65f);
    style.Colors[ImGuiCol_BorderShadow]              = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg]                   = ImVec4(0.44f, 0.80f, 0.80f, 0.18f);
    style.Colors[ImGuiCol_FrameBgHovered]            = ImVec4(0.44f, 0.80f, 0.80f, 0.27f);
    style.Colors[ImGuiCol_FrameBgActive]             = ImVec4(0.44f, 0.81f, 0.86f, 0.66f);
    style.Colors[ImGuiCol_TitleBg]                   = ImVec4(0.14f, 0.18f, 0.21f, 0.73f);
    style.Colors[ImGuiCol_TitleBgActive]             = ImVec4(0.00f, 1.00f, 1.00f, 0.27f);
    style.Colors[ImGuiCol_TitleBgCollapsed]          = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
    style.Colors[ImGuiCol_MenuBarBg]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
    style.Colors[ImGuiCol_ScrollbarBg]               = ImVec4(0.22f, 0.29f, 0.30f, 0.71f);
    style.Colors[ImGuiCol_ScrollbarGrab]             = ImVec4(0.00f, 1.00f, 1.00f, 0.44f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]      = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]       = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]                 = ImVec4(0.00f, 1.00f, 1.00f, 0.68f);
    style.Colors[ImGuiCol_SliderGrab]                = ImVec4(0.00f, 1.00f, 1.00f, 0.36f);
    style.Colors[ImGuiCol_SliderGrabActive]          = ImVec4(0.00f, 1.00f, 1.00f, 0.76f);
    style.Colors[ImGuiCol_Button]                    = ImVec4(0.00f, 0.65f, 0.65f, 0.46f);
    style.Colors[ImGuiCol_ButtonHovered]             = ImVec4(0.01f, 1.00f, 1.00f, 0.43f);
    style.Colors[ImGuiCol_ButtonActive]              = ImVec4(0.00f, 1.00f, 1.00f, 0.62f);
    style.Colors[ImGuiCol_Header]                    = ImVec4(0.00f, 1.00f, 1.00f, 0.33f);
    style.Colors[ImGuiCol_HeaderHovered]             = ImVec4(0.00f, 1.00f, 1.00f, 0.42f);
    style.Colors[ImGuiCol_HeaderActive]              = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
    style.Colors[ImGuiCol_Separator]                 = ImVec4(0.00f, 0.48f, 0.48f, 0.55f);
    style.Colors[ImGuiCol_SeparatorHovered]          = ImVec4(0.00f, 0.48f, 0.48f, 0.55f);
    style.Colors[ImGuiCol_SeparatorActive]           = ImVec4(0.00f, 0.48f, 0.48f, 0.55f);
    style.Colors[ImGuiCol_ResizeGrip]                = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
    style.Colors[ImGuiCol_ResizeGripHovered]         = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
    style.Colors[ImGuiCol_ResizeGripActive]          = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_InputTextCursor]           = ImVec4(0.00f, 0.86f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_TabHovered]                = ImVec4(0.00f, 0.41f, 0.41f, 1.00f);
    style.Colors[ImGuiCol_Tab]                       = ImVec4(0.00f, 0.65f, 0.65f, 0.46f);
    style.Colors[ImGuiCol_TabSelected]               = ImVec4(0.05f, 0.51f, 0.51f, 1.00f);
    style.Colors[ImGuiCol_TabSelectedOverline]       = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
    style.Colors[ImGuiCol_TabDimmed]                 = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_TabDimmedSelected]         = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    style.Colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    style.Colors[ImGuiCol_PlotLines]                 = ImVec4(0.00f, 0.85f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]          = ImVec4(0.00f, 1.00f, 0.59f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]             = ImVec4(0.17f, 0.60f, 0.88f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]      = ImVec4(0.00f, 1.00f, 0.59f, 1.00f);
    style.Colors[ImGuiCol_TableHeaderBg]             = ImVec4(0.05f, 0.78f, 0.78f, 0.58f);
    style.Colors[ImGuiCol_TableBorderStrong]         = ImVec4(0.00f, 0.77f, 0.77f, 1.00f);
    style.Colors[ImGuiCol_TableBorderLight]          = ImVec4(0.04f, 0.83f, 0.83f, 0.75f);
    style.Colors[ImGuiCol_TableRowBg]                = ImVec4(0.00f, 0.42f, 0.42f, 0.48f);
    style.Colors[ImGuiCol_TableRowBgAlt]             = ImVec4(0.00f, 0.65f, 0.65f, 0.46f);
    style.Colors[ImGuiCol_TextLink]                  = ImVec4(0.00f, 0.70f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]            = ImVec4(0.00f, 1.00f, 1.00f, 0.22f);
    style.Colors[ImGuiCol_TreeLines]                 = ImVec4(0.38f, 0.38f, 0.38f, 0.50f);
    style.Colors[ImGuiCol_DragDropTarget]            = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_DragDropTargetBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_UnsavedMarker]             = ImVec4(0.00f, 0.86f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_NavCursor]                 = ImVec4(0.00f, 0.63f, 0.63f, 0.80f);
    style.Colors[ImGuiCol_NavWindowingHighlight]     = ImVec4(0.16f, 0.79f, 0.79f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingDimBg]         = ImVec4(0.00f, 0.29f, 0.24f, 0.40f);
    style.Colors[ImGuiCol_ModalWindowDimBg]          = ImVec4(0.00f, 0.29f, 0.24f, 0.27f);

#ifdef PLATFORM_ANDROID
    // font_scale = ImGui::GetFontSize() / UI_FONT_SIZE; // e.g., 55/22 = 2.5
    ApplyScaleToStyle(style, font_scale);
#endif
}

/*
    ██╗███╗   ██╗████████╗███████╗██████╗ ███╗   ██╗ █████╗ ██╗
    ██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗████╗  ██║██╔══██╗██║
    ██║██╔██╗ ██║   ██║   █████╗  ██████╔╝██╔██╗ ██║███████║██║
    ██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗██║╚██╗██║██╔══██║██║
    ██║██║ ╚████║   ██║   ███████╗██║  ██║██║ ╚████║██║  ██║███████╗
    ╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝

    ███████╗██╗   ██╗███╗   ██╗ ██████╗████████╗██╗ ██████╗ ███╗   ██╗███████╗
    ██╔════╝██║   ██║████╗  ██║██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║██╔════╝
    █████╗  ██║   ██║██╔██╗ ██║██║        ██║   ██║██║   ██║██╔██╗ ██║███████╗
    ██╔══╝  ██║   ██║██║╚██╗██║██║        ██║   ██║██║   ██║██║╚██╗██║╚════██║
    ██║     ╚██████╔╝██║ ╚████║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║███████║
    ╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝

*/

// So many of you wanted this
// And I can relate to such a problem

// Filter only the filenames before showing them on the midi and soundfont lists or it will cause a godamn stupid chaos
std::string FilenameOnly(const std::string &path)
{
    size_t slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? path : path.substr(slash + 1);
}

UI::RGBAint Frgba2Irgba(ImVec4 &col)
{
    UI::RGBAint res;
    res.r = static_cast<int>(col.x * 255.0f);
    res.g = static_cast<int>(col.y * 255.0f);
    res.b = static_cast<int>(col.z * 255.0f);
    res.a = static_cast<int>(col.w * 255.0f);
    return res;
}

bool searchDuplicatedMidiItem(const std::vector<std::string> &items, const std::string &item)
{
    for(const auto &i : items)
        if(i == item)
            return true;
    return false;
}

bool searchDuplicatedSoundfontItem(const std::vector<UI::SoundfontItem> &items, const std::string &item)
{
    for(size_t i = 0; i < items.size(); ++i)
        if(items[i].label == item)
        {
            selected_soundfont = i;
            return true;
        }
    return false;
}

void RenderMidiList(const std::vector<std::string> &items, int &selectedIndex, std::string find_item)
{
    static KineticState ks_midi_y;
    static KineticState ks_midi_x;
    ImGui::BeginChild("##midils", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

#ifdef PLATFORM_ANDROID
    ImGuiIO &io = ImGui::GetIO();
    {
        bool hovered  = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        bool dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
        ImGui::SetScrollY((f32)ks_midi_y.Update(ImGui::GetScrollY(), io.MouseDelta.y, io.DeltaTime, hovered, dragging));
        ImGui::SetScrollX((f32)ks_midi_x.Update(ImGui::GetScrollX(), io.MouseDelta.x, io.DeltaTime, hovered, dragging));
    }
#endif

    for(size_t i = 0; i < items.size(); ++i)
    {
        std::string midi_filename = FilenameOnly(items[i]);

        // Filter check (case-insensitive optional)
        if(!find_item.empty() && midi_filename.find(find_item) == std::string::npos)
            continue;

        bool isSelected = (i == static_cast<size_t>(selectedIndex));

        if(ImGui::Selectable((midi_filename + "##" + std::to_string(i)).c_str(), isSelected))
            selectedIndex = static_cast<int>(i);

        if(isSelected)
            ImGui::SetItemDefaultFocus();
    }
    ImGui::EndChild();
}

std::vector<std::string> GetCheckedSoundfonts(const std::vector<UI::SoundfontItem> &items)
{
    std::vector<std::string> checkedItems;

    for(const auto &item : items)
        if(item.checked)
            checkedItems.push_back(item.label);

    return checkedItems;
}

void RenderSoundfontList(std::vector<UI::SoundfontItem> &items, std::string find_item)
{
    static KineticState ks_sf_y;
    static KineticState ks_sf_x;
    ImGui::BeginChild("##sfls", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

#ifdef PLATFORM_ANDROID
    ImGuiIO &io = ImGui::GetIO();
    {
        bool hovered  = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        bool dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
        ImGui::SetScrollY((f32)ks_sf_y.Update(ImGui::GetScrollY(), io.MouseDelta.y, io.DeltaTime, hovered, dragging));
        ImGui::SetScrollX((f32)ks_sf_x.Update(ImGui::GetScrollX(), io.MouseDelta.x, io.DeltaTime, hovered, dragging));
    }
#endif

    bool soundfont_changed = false;

    for(size_t i = 0; i < items.size(); ++i)
    {
        std::string sf_filename = FilenameOnly(items[i].label);

        // Filter check
        if(!find_item.empty() && sf_filename.find(find_item) == std::string::npos)
            continue;

        // Store previous checkbox state
        bool        previous_state = items[i].checked;

        ImGuiStyle &style          = ImGui::GetStyle();


#ifdef PLATFORM_ANDROID
        const int item_selection_height = 61;
#else
        // const int item_selection_height = 28;
        static int item_selection_height = (int)style.FramePadding.y + 27;
#endif

        std::string temp = "##" + std::to_string(i);
        if(ImGui::Selectable(temp.c_str(), selected_soundfont == (int)i, ImGuiSelectableFlags_AllowOverlap, ImVec2(0, item_selection_height)))
            selected_soundfont = i;

        ImGui::SameLine();

        // Unique ID to avoid conflicts and update the single selection index too for better UX
        if(ImGui::Checkbox((sf_filename + "##" + std::to_string(i)).c_str(), &items[i].checked))
            selected_soundfont = i;

        // Check if state changed
        if(previous_state != items[i].checked)
            soundfont_changed = true;

        if(selected_soundfont == (int)i && trigger_scroll_sf_list)
        {
            ImGui::ScrollToItem(ImGuiScrollFlags_AlwaysCenterY);
            trigger_scroll_sf_list = false;
        }
    }

    ImGui::EndChild();

    // If any soundfont selection changed, update the checked_soundfonts list
    // and reload soundfonts immediately
    if(soundfont_changed)
    {
        checked_soundfonts = GetCheckedSoundfonts(items);
        SoundfontList::Save(live_soundfont_list);
        Playback::ReloadSoundfonts();
    }
}

/*
void RenderSoundfontsPathsList(const std::vector<std::string> &items, int &selectedIndex)
{
    ImGui::BeginChild("##soundfontspathls", ImVec2(0, 230), true, ImGuiWindowFlags_HorizontalScrollbar);

    static size_t sel_idx = static_cast<size_t>(selectedIndex);
    for(size_t i = 0; i < items.size(); ++i)
    {
        bool isSelected = (i == sel_idx);

        if(ImGui::Selectable((items[i] + "##" + std::to_string(i)).c_str(), isSelected))
            sel_idx = i;

        if(isSelected)
            ImGui::SetItemDefaultFocus();
    }

    ImGui::EndChild();
}
*/

void ShowAudioDeviceList(const std::vector<Playback::AudioDevice> &audioDevices)
{
    std::vector<const char *> deviceNames;
    for(const auto &device : audioDevices)
        deviceNames.push_back(device.name);

    // Ensure current_audio_dev is valid
    if(deviceNames.empty())
    {
        UI::current_audio_dev = 0; // Reset to 0 if there are no devices
        allow_audio_dev_ssave = false;
    }
    else if(UI::current_audio_dev >= (int)deviceNames.size())
    {
        allow_audio_dev_ssave = true;
        UI::current_audio_dev = 0; // Reset to the first device if the index is out of bounds
    }

    // Display the combo box
    const char *currentDeviceName = (deviceNames.empty() ? "No devices available" : deviceNames[UI::current_audio_dev]);

    if(ImGui::BeginCombo("Audio Devices *", currentDeviceName))
    {
        if(deviceNames.empty())
        {
            // Render a placeholder item when no devices are available
            ImGui::Selectable("No devices available", false);
        }
        else
        {
            for(size_t i = 0; i < deviceNames.size(); i++)
            {
                // Check if this item is selected
                bool isSelected = (UI::current_audio_dev == (int)i);

                // Add the item to the combo box
                if(ImGui::Selectable(deviceNames[i], isSelected))
                    UI::current_audio_dev = i; // Update the current index if the user selects this item

                // Set the initial focus when opening the combo box
                if(isSelected)
                    ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

us_int ImVec4ToUInt(const ImVec4 &color)
{
    us_int r = static_cast<us_int>(color.x * 255.0f);
    us_int g = static_cast<us_int>(color.y * 255.0f);
    us_int b = static_cast<us_int>(color.z * 255.0f);
    return (r << 16) | (g << 8) | b;
}

void moveSoundfont(int index, int direction)
{
    int new_index = index + direction;
    if(index < 0 || index >= (int)live_soundfont_list.size())
        return;
    if(new_index < 0 || new_index >= (int)live_soundfont_list.size())
        return;

    std::swap(live_soundfont_list[index], live_soundfont_list[new_index]);
}

void ConstrainWindowMove(const char *windowName)
{
    ImGuiContext &g   = *GImGui;

    ImGuiWindow  *win = ImGui::FindWindowByName(windowName);
    if(g.MovingWindow != nullptr)
    {
        ImGuiWindow *movingWin = g.MovingWindow;

        // Find the root (non-child) window
        while(movingWin->ParentWindow != nullptr)
            movingWin = movingWin->ParentWindow;

        // Check if this root window matches our target
        if(movingWin != win)
            return;

        ImVec2 viewportPos  = movingWin->Viewport->Pos;
        ImVec2 viewportSize = movingWin->Viewport->Size;
        ImVec2 windowSize   = movingWin->Size;

        ImVec2 minPos       = viewportPos;
        ImVec2 maxPos;
        maxPos.x   = viewportPos.x + viewportSize.x - windowSize.x;
        maxPos.y   = viewportPos.y + viewportSize.y - windowSize.y;

        ImVec2 pos = movingWin->Pos;

        pos.x      = std::clamp(pos.x, minPos.x, maxPos.x);
        pos.y      = std::clamp(pos.y, minPos.y, maxPos.y);

        if(pos.x != movingWin->Pos.x || pos.y != movingWin->Pos.y)
            movingWin->Pos = pos;
    }
}

void AndroidUIRescale()
{
    const float base_font_size        = UI_FONT_SIZE;
    float       scale                 = ImGui::GetFontSize() / base_font_size;


    ImGuiStyle &style                 = ImGui::GetStyle();


    style.Alpha                       = 1.0f;
    style.WindowRounding              = 10.0f;
    style.ChildRounding               = 6.0f;
    style.FrameRounding               = 6.0f;
    style.PopupRounding               = 10.0f;
    style.GrabRounding                = 3.0f;
    style.ScrollbarRounding           = 4.0f;
    style.TabRounding                 = 3.0f;
    style.WindowPadding               = ImVec2(14, 14);
    style.FramePadding                = ImVec2(12, 5);
    style.ItemSpacing                 = ImVec2(8, 7);
    style.ItemInnerSpacing            = ImVec2(6, 4);
    style.IndentSpacing               = 21.0f;
    style.ScrollbarSize               = 28.0f;
    style.ScrollbarPadding            = 6.0f;
    style.GrabMinSize                 = 12.0f;
    style.WindowBorderSize            = 1.0f;
    style.ChildBorderSize             = 1.0f;
    style.PopupBorderSize             = 1.0f;
    style.FrameBorderSize             = 1.0f;
    style.TabBorderSize               = 1.0f;
    style.TabBarBorderSize            = 2.0f;
    style.TabRounding                 = 8.0f;
    style.CellPadding                 = ImVec2(8, 12);
    style.TableAngledHeadersTextAlign = ImVec2(0.50, 0.50);
    style.WindowTitleAlign            = ImVec2(0.50, 0.50);
    style.WindowBorderHoverPadding    = 6.0f;
    style.WindowMenuButtonPosition    = ImGuiDir_Right;
    style.ColorMarkerSize             = 8.0f;
}

void SetupIconFonts()
{
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig         icons_config;
    icons_config.MergeMode            = true;
    icons_config.PixelSnapH           = true;
    icons_config.FontDataOwnedByAtlas = false;
    f32 size                          = FONT_AWESOME_ICON_SIZE;

    icons_config.GlyphMaxAdvanceX     = std::numeric_limits<f32>::max();
    icons_config.RasterizerMultiply   = 1.0f;
    icons_config.OversampleH          = 2;
    icons_config.OversampleV          = 1;

    icons_config.GlyphRanges          = icons_ranges;

    ImGuiIO &io                       = ImGui::GetIO();
    io.Fonts->AddFontFromMemoryCompressedTTF((void *)fa_solid_900_compressed_data, fa_solid_900_compressed_size, size, &icons_config, icons_ranges);
}

// - - - - [End of Internal Functions] - - - -

/*
    ███████╗██╗  ██╗████████╗███████╗██████╗ ███╗   ██╗ █████╗ ██╗
    ██╔════╝╚██╗██╔╝╚══██╔══╝██╔════╝██╔══██╗████╗  ██║██╔══██╗██║
    █████╗   ╚███╔╝    ██║   █████╗  ██████╔╝██╔██╗ ██║███████║██║
    ██╔══╝   ██╔██╗    ██║   ██╔══╝  ██╔══██╗██║╚██╗██║██╔══██║██║
    ███████╗██╔╝ ██╗   ██║   ███████╗██║  ██║██║ ╚████║██║  ██║███████╗
    ╚══════╝╚═╝  ╚═╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝

    ███████╗██╗   ██╗███╗   ██╗ ██████╗████████╗██╗ ██████╗ ███╗   ██╗███████╗
    ██╔════╝██║   ██║████╗  ██║██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║██╔════╝
    █████╗  ██║   ██║██╔██╗ ██║██║        ██║   ██║██║   ██║██╔██╗ ██║███████╗
    ██╔══╝  ██║   ██║██║╚██╗██║██║        ██║   ██║██║   ██║██║╚██╗██║╚════██║
    ██║     ╚██████╔╝██║ ╚████║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║███████║
    ╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝

*/

ImVec4 UI::Irgba2ImVec4(int r, int g, int b, int a)
{
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

void UI::SetBuiltinTheme(int idx)
{
    if(idx < 0 || idx > 5)
    {
        Log::warn("builtin theme index out of range: %d", idx);
        return;
    }

    if(idx == 0)
        SetSilvanaTheme();
    else if(idx == 1)
        SetMoonlightTheme();
    else if(idx == 2)
        SetNeonAbyssTheme();
    else if(idx == 3)
        SetCrimsonAzureTheme();
    else if(idx == 4)
        SetArcticHorizonTheme();
    else if(idx == 5)
        SetCyberpunkTheme();

    text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

ImVec4 UI::UIntToImVec4(us_int rgb)
{
    f32 alpha = 1.0f;
    f32 r     = ((rgb >> 16) & 0xFF) / 255.0f;
    f32 g     = ((rgb >> 8) & 0xFF) / 255.0f;
    f32 b     = (rgb & 0xFF) / 255.0f;
    return ImVec4(r, g, b, alpha);
}

void UI::UpdateWidgetValues()
{
    UI::clear_color = UI::Irgba2ImVec4(live_conf.bg_R, live_conf.bg_G, live_conf.bg_B, live_conf.bg_A);

    UI::liveColor.r = live_conf.bg_R;
    UI::liveColor.g = live_conf.bg_G;
    UI::liveColor.b = live_conf.bg_B;
    UI::liveColor.a = live_conf.bg_A;

    if(std::filesystem::exists(live_conf.last_ccol_file_path))
    {
        unsigned int *colors          = ChannelColors::getChannelColors(live_conf.last_ccol_file_path);
        live_conf.is_custom_ch_colors = true;

        if(colors != nullptr)
            std::copy(colors, colors + 16, live_conf.channel_colors);
        else
            std::copy(default_settings.channel_colors, default_settings.channel_colors + 16, live_conf.channel_colors);

        delete[] colors;
    }
    else
        std::copy(default_settings.channel_colors, default_settings.channel_colors + 16, live_conf.channel_colors);


    for(size_t i = 0; i < live_midi_list.size(); i++)
    {
        if(live_midi_list[i] == MidiList::last_midi_file)
        {
            selected_midi_index = (int)i;
            break;
        }
    }


    UI::current_audio_dev       = live_conf.audio_device_index;
    UI::loop_colors             = live_conf.loop_colors;
    UI::overlap_remover         = live_conf.OR;
    UI::velocity_filter         = live_conf.vel_filter;
    UI::live_note_speed         = live_conf.note_speed;
    UI::min_velocity            = live_conf.vel_min;
    UI::max_velocity            = live_conf.vel_max;
    UI::last_midi_path          = live_fd_state.midi_path;
    UI::last_midi_file          = MidiList::last_midi_file;
    UI::vsync                   = live_conf.vsync;
    UI::internal_logging        = live_conf.internal_log_buffer;
    UI::log_to_file             = live_conf.log_to_file;
    UI::fps                     = live_conf.fps;
    //UI::soundfont_paths         = live_conf.extra_sf_paths;
    UI::last_sf_path            = live_fd_state.soundfont_path;
    UI::no_midi_duplicates      = live_conf.no_midi_duplicates;
    UI::vertical_lines          = live_conf.draw_vertical_lines;
    UI::draw_measure_lines      = live_conf.draw_measure_lines;
    UI::no_soundfont_duplicates = live_conf.no_soundfont_duplicates;
    UI::background_image        = live_conf.background_image;
    UI::ui_theming              = live_conf.custom_ui_theme;
    live_soundfont_list         = loaded_soundfont_list;
}

void UI::Setup(int graphics_backend)
{
    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

#ifdef PLATFORM_ANDROID
    io.IniFilename = IMGUI_INI_FILE_PATH;
    if(!std::filesystem::exists(IMGUI_INI_FILE_PATH))
        Log::warn("", "Previous UI layout is no longer available. Missing '%s'", IMGUI_INI_FILE_PATH);
#else
    std::ostringstream ini_path;
    ini_path << FileHelpers::config_dir << "/" << IMGUI_INI_FILE_PATH;
    static std::string temp_str = ini_path.str();
    io.IniFilename              = temp_str.c_str();
    if(!std::filesystem::exists(temp_str.c_str()))
        Log::warn("", "Previous UI layout is no longer available. Missing '%s'", temp_str.c_str());
#endif

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls



    // Metrophobic-Regular.ttf
    // Font suggested by Nerdly

    ImFontConfig ui_font_config;
    ui_font_config.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF((void *)metrophobic_regular_ttf, metrophobic_regular_len, UI_FONT_SIZE, &ui_font_config);
    // io.Fonts->AddFontFromMemoryTTF((void *)metrophobic_regular_ttf, metrophobic_regular_len, 55.0f, &ui_font_config);

    // font_scale = ImGui::GetFontSize() / UI_FONT_SIZE;
    font_scale = UI_FONT_SIZE / 62.0f;

    SetupIconFonts();


    // Setup ImGui style
    ImGui::StyleColorsDark();

    builtin_ui_theme_idx = live_conf.builtin_ui_theme_idx;

    if(live_conf.custom_ui_theme)
        // SetDefaultTheme(); // Setting a nice looking GUI :3
        // SetSilvanaTheme();
        ImGui::LoadStyleFrom(live_conf.ui_theme_file_path.c_str());

    if(live_conf.builtin_ui_theme)
        SetBuiltinTheme(live_conf.builtin_ui_theme_idx);

    // Setup Platform/Renderer backends
    // ImGui_ImplSDL3_InitForSDLRenderer(w, r);
    ImGui_ImplSDLRenderer3_Init(RenderWin->Ren);
    ImGui_ImplSDL3_InitForVulkan(RenderWin->Win);

    UpdateWidgetValues();
}

void UI::Render(SDL_Renderer *r)
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGuiIO &io = ImGui::GetIO();

    ImVec2   buton_sizes;

    // Wider buttons for the lists
#ifdef PLATFORM_ANDROID
    buton_sizes = ImVec2(150, 0);
#else
    buton_sizes = ImVec2(0, 0);
#endif



    ImVec2    displaySize        = io.DisplaySize;
    const f32 longClickThreshold = 0.5f;

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if(show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // Reserved for future use
    // ImGui::Text("%.1f FPS", nvg.io.Framerate);

    /*
        ▗▄▄▖ ▗▖    ▗▄▖▗▖  ▗▖▗▄▄▄▖▗▄▄▖
        ▐▌ ▐▌▐▌   ▐▌ ▐▌▝▚▞▘ ▐▌   ▐▌ ▐▌
        ▐▛▀▘ ▐▌   ▐▛▀▜▌ ▐▌  ▐▛▀▀▘▐▛▀▚▖
        ▐▌   ▐▙▄▄▖▐▌ ▐▌ ▐▌  ▐▙▄▄▖▐▌ ▐▌



         ▗▄▄▖ ▗▄▖ ▗▖  ▗▖▗▄▄▄▖▗▄▄▖  ▗▄▖ ▗▖    ▗▄▄▖
        ▐▌   ▐▌ ▐▌▐▛▚▖▐▌  █  ▐▌ ▐▌▐▌ ▐▌▐▌   ▐▌
        ▐▌   ▐▌ ▐▌▐▌ ▝▜▌  █  ▐▛▀▚▖▐▌ ▐▌▐▌    ▝▀▚▖
        ▝▚▄▄▖▝▚▄▞▘▐▌  ▐▌  █  ▐▌ ▐▌▝▚▄▞▘▐▙▄▄▖▗▄▄▞▘



    */

    ImVec2 windowPos = ImVec2((displaySize.x - displaySize.x + 15) * 0.5f, (displaySize.y - displaySize.y + 15) * 0.5f);
    ImGui::SetNextWindowSize(ImVec2(displaySize.x - 15, displaySize.y - 15)); // No size constraints
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::Begin("InvisibleWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::Columns(3, "ResizableColumns", false); // 2 columns, resizable
    f32 sb_col_w = ImGui::GetColumnWidth();
    f32 sb_btn_h = ImGui::GetContentRegionAvail().y;
    if(ImGui::InvisibleButton("<<", ImVec2(sb_col_w, sb_btn_h)))
        Playback::seek_playback(-Playback::seek_amount);

    if(ImGui::IsItemActive())
    {
        f32 holdDuration = ImGui::GetIO().MouseDownDuration[0]; // [0] is for the left mouse button

        // Handling long click / tap event
        if(holdDuration > longClickThreshold)
            main_gui_window = true;
    }

    ImGui::NextColumn();

    f32 ps_col_w = ImGui::GetColumnWidth();
    f32 ps_btn_h = ImGui::GetContentRegionAvail().y;
    if(ImGui::InvisibleButton(Playback::is_paused ? "|>" : "||", ImVec2(ps_col_w, ps_btn_h)))
        Playback::pause();

    if(ImGui::IsItemActive())
    {
        if(ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            f32 dragDelta = io.MouseDelta.x;
            if(fabsf(dragDelta) > 2.0f)
                Playback::seek_playback(-dragDelta * 0.05);
        }

        f32 holdDuration = ImGui::GetIO().MouseDownDuration[0];

        if(holdDuration > longClickThreshold)
            main_gui_window = true;
    }

    ImGui::NextColumn();

    f32 sf_col_w = ImGui::GetColumnWidth();
    f32 sf_btn_h = ImGui::GetContentRegionAvail().y;
    if(ImGui::InvisibleButton(">>", ImVec2(sf_col_w, sf_btn_h)))
        Playback::seek_playback(Playback::seek_amount);

    if(ImGui::IsItemActive())
    {
        f32 holdDuration = ImGui::GetIO().MouseDownDuration[0]; // [0] is for the left mouse button

        // Handling long click / tap event to open up the settings window
        if(holdDuration > longClickThreshold)
            main_gui_window = true;
    }

    ImGui::Columns(1); // Reset to single column



    /*
        ▗▖  ▗▖ ▗▄▖ ▗▄▄▄▖▗▖  ▗▖
        ▐▛▚▞▜▌▐▌ ▐▌  █  ▐▛▚▖▐▌
        ▐▌  ▐▌▐▛▀▜▌  █  ▐▌ ▝▜▌
        ▐▌  ▐▌▐▌ ▐▌▗▄█▄▖▐▌  ▐▌


        ▗▖ ▗▖▗▄▄▄▖▗▖  ▗▖▗▄▄▄  ▗▄▖ ▗▖ ▗▖
        ▐▌ ▐▌  █  ▐▛▚▖▐▌▐▌  █▐▌ ▐▌▐▌ ▐▌
        ▐▌ ▐▌  █  ▐▌ ▝▜▌▐▌  █▐▌ ▐▌▐▌ ▐▌
        ▐▙█▟▌▗▄█▄▖▐▌  ▐▌▐▙▄▄▀▝▚▄▞▘▐▙█▟▌



    */



    ConstrainWindowMove("PFA Android");

    // Show the main GUI window
    if(main_gui_window)
    {
        ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
#ifndef PLATFORM_ANDROID
        ImGui::SetNextWindowSizeConstraints(ImVec2(758, 380), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin("PFA Android", &main_gui_window);
#else // Setting up a different ui layout for mobile users
        ImGui::SetNextWindowPos(center, ImGuiCond_Once, ImVec2(0.5f, 0.5f)); // Pivot 0.5 = center
        ImGui::SetNextWindowSize(ImVec2(1500.0f, 860.0f));
        ImGui::Begin("PFA Android", &main_gui_window, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_HorizontalScrollbar);
#endif

        ImGuiIO    &io             = ImGui::GetIO();
        ImGuiStyle &internal_style = ImGui::GetStyle();

#ifdef PLATFORM_ANDROID
        {
            static KineticState ks_gui_y;
            static KineticState ks_gui_x;
            bool hovered = ImGui::IsWindowHovered();
            bool dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
            ImGui::SetScrollY((f32)ks_gui_y.Update(ImGui::GetScrollY(), io.MouseDelta.y, io.DeltaTime, hovered, dragging));
            ImGui::SetScrollX((f32)ks_gui_x.Update(ImGui::GetScrollX(), io.MouseDelta.x, io.DeltaTime, hovered, dragging));
        }
#endif



        velocity_filter = live_conf.vel_filter;
        min_velocity    = live_conf.vel_min;
        max_velocity    = live_conf.vel_max;

        if(ImGui::Button("Quit"))
            // Exit();
            VisualizerHandler::shutdown();

        if(ImGui::BeginItemTooltip())
        {
            ImGui::Text("Quit NVi PFA");
            ImGui::EndTooltip();
        }

        if(ImGui::BeginTabBar("tabs", ImGuiTabBarFlags_None))
        {
            /*
                ▗▖  ▗▖▗▄▄▄▖▗▄▄▄ ▗▄▄▄▖    ▗▖   ▗▄▄▄▖ ▗▄▄▖▗▄▄▄▖
                ▐▛▚▞▜▌  █  ▐▌  █  █      ▐▌     █  ▐▌     █
                ▐▌  ▐▌  █  ▐▌  █  █      ▐▌     █   ▝▀▚▖  █
                ▐▌  ▐▌▗▄█▄▖▐▙▄▄▀▗▄█▄▖    ▐▙▄▄▖▗▄█▄▖▗▄▄▞▘  █



                ▗▄▄▄▖▗▄▖ ▗▄▄▖
                  █ ▐▌ ▐▌▐▌ ▐▌
                  █ ▐▛▀▜▌▐▛▀▚▖
                  █ ▐▌ ▐▌▐▙▄▞▘



            */
            if(ImGui::BeginTabItem("Play MIDI Files"))
            {
#ifdef PLATFORM_ANDROID
                ImGui::SetNextItemWidth(500);
#else
                ImGui::SetNextItemWidth(310);
#endif
                ImGui::InputTextWithHint("##EHE", "Search midis", midi_search, IM_ARRAYSIZE(midi_search));

                midi_search_text = midi_search;

                ImGui::SameLine();

                static bool button_click;
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(60, 177, 195, 255));
                if(ImGui::Button(ICON_FA_FOLDER_OPEN, buton_sizes))
                    button_click = true;
                else
                    button_click = false;
                ImGui::PopStyleColor();

                if(button_click)
                {
                    IGFD::FileDialogConfig config;
                    config.path  = live_fd_state.midi_path;
                    config.flags = ImGuiFileDialogFlags_HideColumnType;

                    // const char* group_name = "Places";

                    ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".mid", internal_style.Colors[ImGuiCol_Text], ICON_FA_MUSIC);
                    ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".midi", internal_style.Colors[ImGuiCol_Text], ICON_FA_MUSIC);
                    ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".smf", internal_style.Colors[ImGuiCol_Text], ICON_FA_MUSIC);
                    ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".MID", internal_style.Colors[ImGuiCol_Text], ICON_FA_MUSIC);
                    ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".MIDI", internal_style.Colors[ImGuiCol_Text], ICON_FA_MUSIC);
                    ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".SMF", internal_style.Colors[ImGuiCol_Text], ICON_FA_MUSIC);

                    // Create the group if it doesn't exist


                    ImGuiFileDialog::Instance()->OpenDialog("MidiFileFD", "Choose a MIDI File", ".mid,.midi,.smf,.MID,.MIDI,.SMF", config);

                    ImGuiFileDialog::Instance()->AddPlacesGroup("Places", 0, false, true);
                    // Then get the pointer
                    auto places_ptr = ImGuiFileDialog::Instance()->GetPlacesGroupPtr("Places");
                    if(places_ptr)
                    {
#ifdef PLATFORM_ANDROID
                        IGFD::FileStyle f_style_home;
                        f_style_home.color = internal_style.Colors[ImGuiCol_Text];
                        f_style_home.icon  = ICON_FA_HOUSE;

                        places_ptr->AddPlace("Home", "/storage/emulated/0", false, f_style_home);
#else
                        IGFD::FileStyle f_style_home;
                        f_style_home.color = internal_style.Colors[ImGuiCol_Text];
                        f_style_home.icon  = ICON_FA_HOUSE;

                        IGFD::FileStyle f_style_downloads;
                        f_style_downloads.color = internal_style.Colors[ImGuiCol_Text];
                        f_style_downloads.icon  = ICON_FA_DOWNLOAD;

                        places_ptr->AddPlace("Home", "/home/andre/Desktop", false, f_style_home);
                        places_ptr->AddPlace("Downloads", "/home/andre/Downloads", false, f_style_downloads);
#endif
                    }
                }


                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Open and play a MIDI file");
                    ImGui::EndTooltip();
                }

                ImGui::PushFont(FONT_icon_set);
                ConstrainWindowMove("Choose a MIDI File##MidiFileFD"); // Very smart tweaking XDD (See ImGuiFileDialog.cpp:3841)
                if(ImGuiFileDialog::Instance()->Display("MidiFileFD", 0, ImVec2(700, 500), ImVec2(FLT_MAX, FLT_MAX)))
                {
                    if(ImGuiFileDialog::Instance()->IsOk())
                    {
                        midi_file               = ImGuiFileDialog::Instance()->GetFilePathName();
                        live_fd_state.midi_path = ImGuiFileDialog::Instance()->GetCurrentPath();
                        FileDialogState::save(live_fd_state);
                        Playback::CloseMidi(); // Close previous MIDI file
                        ImGui::OpenPopup("Loading MIDI...");
                        Playback::loadMidiFile(midi_file);

                        if(no_midi_duplicates)
                        {
                            if(!searchDuplicatedMidiItem(live_midi_list, midi_file))
                            {
                                live_midi_list.emplace_back(midi_file);
                                MidiList::save(live_midi_list, midi_file);
                                selected_midi_index++;
                            }
                            else
                                Log::info("Same midi already exists");
                        }
                        else
                        {
                            live_midi_list.emplace_back(midi_file);
                            selected_midi_index++;
                            MidiList::save(live_midi_list, midi_file);
                        }
                    }

                    ImGuiFileDialog::Instance()->Close();
                }
                ImGui::PopFont();

                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(51, 204, 51, 255));
                if(ImGui::Button(ICON_FA_PLAY, buton_sizes))
                {
                    Playback::CloseMidi();
                    MidiList::last_midi_file = live_midi_list[selected_midi_index];
                    MidiList::save(live_midi_list, MidiList::last_midi_file);
                    ImGui::OpenPopup("Loading MIDI...");
                    Playback::loadMidiFile(live_midi_list[selected_midi_index]);
                }
                ImGui::PopStyleColor();

                // Show this very useful popup while loading MIDI (Very luxurious feature tbh)
                if(ImGui::BeginPopupModal("Loading MIDI...", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    char        buf[32];
                    const float progress = static_cast<float>(Midi_ctx.MIDI_File.current_loaded_track) / static_cast<float>(Midi_ctx.MIDI_File.tracks - 1);

                    sprintf(buf, "Loading track: %d/%d", (int)(progress * Midi_ctx.MIDI_File.tracks - 1), Midi_ctx.MIDI_File.tracks - 1);
                    ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), buf);

                    if(Playback::is_midi_stream_creating)
                        ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(0.0f, 0.0f), "Creating MIDI Stream...");

                    if(Playback::is_midi_loaded)
                    {
                        ImGui::CloseCurrentPopup();
                        main_gui_window = false;
                    }

                    ImGui::EndPopup();
                }

                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Play the previous MIDI file");
                    ImGui::EndTooltip();
                }

                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 92, 51, 255));
                if(ImGui::Button(ICON_FA_SQUARE, buton_sizes))
                    Playback::CloseMidi();
                ImGui::PopStyleColor();

                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Close and reset midi playback");
                    ImGui::EndTooltip();
                }

                ImGui::SameLine();

                ImGui::BeginDisabled(live_midi_list.size() == 0);
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(179, 179, 179, 255));
                if(ImGui::Button(ICON_FA_TRASH_CAN, buton_sizes))
                {
                    live_midi_list.erase(live_midi_list.begin() + selected_midi_index);
                    MidiList::save(live_midi_list, midi_file);
                }
                ImGui::PopStyleColor();
                ImGui::EndDisabled();

                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Remove previous midi from the list");
                    ImGui::EndTooltip();
                }

                ImGui::SameLine();

                ImGui::BeginDisabled(live_midi_list.size() == 0);
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                if(ImGui::Button(ICON_FA_RECTANGLE_XMARK, buton_sizes))
                    ImGui::OpenPopup("Clear Midi List Confirmation");
                ImGui::PopStyleColor();
                ImGui::EndDisabled();


                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Clear midi list");
                    ImGui::EndTooltip();
                }

                ImGui::SameLine();

                ImGui::BeginDisabled(live_midi_list.size() == 0);
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(51, 153, 255, 255));
                if(ImGui::Button(ICON_FA_CIRCLE_INFO, buton_sizes))
                {
                    current_file_info = FileHelpers::GetFileInfo(live_midi_list[selected_midi_index]);

                    strncpy(file_name_buf, current_file_info.file_name.c_str(), sizeof(file_name_buf) - 1);
                    strncpy(file_size_buf, current_file_info.size.c_str(), sizeof(file_size_buf) - 1);
                    strncpy(last_mod_buf, current_file_info.last_mod.c_str(), sizeof(last_mod_buf) - 1);

                    cached_midi_info  = FileHelpers::ParseMidiFile(live_midi_list[selected_midi_index]);
                    show_midi_details = false;

                    file_info_window  = true;
                    is_midi_info      = true;
                }
                ImGui::PopStyleColor();
                ImGui::EndDisabled();

                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Show file information");
                    ImGui::EndTooltip();
                }

                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

                // ConstrainWindowMove("Clear Midi List Confirmation"); // This ain't workin' bruh

                if(ImGui::BeginPopupModal("Clear Midi List Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Prevous MIDI list is about to be cleared. This operation has no undo !!");
                    ImGui::Text("Are you sure you want to proceed?");

                    if(ImGui::Button("No", ImVec2(120.0f, 0)))
                        ImGui::CloseCurrentPopup();

                    ImGui::SameLine();

                    if(ImGui::Button("Yes", ImVec2(120.0f, 0)))
                    {
                        live_midi_list.clear();
                        MidiList::save(live_midi_list, midi_file);
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                RenderMidiList(live_midi_list, selected_midi_index, midi_search_text);
                ImGui::EndTabItem();
            }
            /*
                ▗▄▄▖ ▗▄▖ ▗▖ ▗▖▗▖  ▗▖▗▄▄▄ ▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖▗▄▄▄▖    ▗▖   ▗▄▄▄▖ ▗▄▄▖▗▄▄▄▖
               ▐▌   ▐▌ ▐▌▐▌ ▐▌▐▛▚▖▐▌▐▌  █▐▌   ▐▌ ▐▌▐▛▚▖▐▌  █      ▐▌     █  ▐▌     █
                ▝▀▚▖▐▌ ▐▌▐▌ ▐▌▐▌ ▝▜▌▐▌  █▐▛▀▀▘▐▌ ▐▌▐▌ ▝▜▌  █      ▐▌     █   ▝▀▚▖  █
               ▗▄▄▞▘▝▚▄▞▘▝▚▄▞▘▐▌  ▐▌▐▙▄▄▀▐▌   ▝▚▄▞▘▐▌  ▐▌  █      ▐▙▄▄▖▗▄█▄▖▗▄▄▞▘  █



               ▗▄▄▄▖▗▄▖ ▗▄▄▖
                 █ ▐▌ ▐▌▐▌ ▐▌
                 █ ▐▛▀▜▌▐▛▀▚▖
                 █ ▐▌ ▐▌▐▙▄▞▘



            */
            if(ImGui::BeginTabItem("Soundfonts"))
            {
#ifdef PLATFORM_ANDROID
                ImGui::SetNextItemWidth(500);
#else
                ImGui::SetNextItemWidth(310);
#endif
                ImGui::InputTextWithHint("##XD", "Search soundfonts", sf_search, IM_ARRAYSIZE(sf_search));

                sf_search_text = sf_search;

                ImGui::SameLine();

                static bool button_click;
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
                if(ImGui::Button(ICON_FA_SQUARE_PLUS, buton_sizes))
                    button_click = true;
                else
                    button_click = false;
                ImGui::PopStyleColor();

                if(button_click)
                {
                    IGFD::FileDialogConfig config;
                    config.path  = live_fd_state.soundfont_path;
                    config.flags = ImGuiFileDialogFlags_HideColumnType;
                    ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".sf2", internal_style.Colors[ImGuiCol_Text], ICON_FA_FILE_AUDIO);
                    ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".sfz", internal_style.Colors[ImGuiCol_Text], ICON_FA_FILE_CODE);
                    ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".SF2", internal_style.Colors[ImGuiCol_Text], ICON_FA_FILE_AUDIO);
                    ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".SFZ", internal_style.Colors[ImGuiCol_Text], ICON_FA_FILE_CODE);
                    
                    ImGuiFileDialog::Instance()->OpenDialog("SoundfontFD", "Choose Soundfont File", ".sf2,.sfz,.SF2,.SFZ", config);

                    ImGuiFileDialog::Instance()->AddPlacesGroup("Places", 0, false, true);
                    // Then get the pointer
                    auto places_ptr = ImGuiFileDialog::Instance()->GetPlacesGroupPtr("Places");
                    if(places_ptr)
                    {
#ifdef PLATFORM_ANDROID
                        IGFD::FileStyle f_style_home;
                        f_style_home.color = internal_style.Colors[ImGuiCol_Text];
                        f_style_home.icon  = ICON_FA_HOUSE;

                        places_ptr->AddPlace("Home", "/storage/emulated/0", false, f_style_home);
#else
                        IGFD::FileStyle f_style_home;
                        f_style_home.color = internal_style.Colors[ImGuiCol_Text];
                        f_style_home.icon  = ICON_FA_HOUSE;

                        IGFD::FileStyle f_style_downloads;
                        f_style_downloads.color = internal_style.Colors[ImGuiCol_Text];
                        f_style_downloads.icon  = ICON_FA_DOWNLOAD;

                        places_ptr->AddPlace("Home", "/home/andre/Desktop", false, f_style_home);
                        places_ptr->AddPlace("Downloads", "/home/andre/Downloads", false, f_style_downloads);
#endif
                    }
                }

                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Add new soundfont to the list");
                    ImGui::EndTooltip();
                }

                ConstrainWindowMove("Choose Soundfont File##SoundfontFD");
                if(ImGuiFileDialog::Instance()->Display("SoundfontFD", 0, ImVec2(700, 500), ImVec2(FLT_MAX, FLT_MAX)))
                {
                    if(ImGuiFileDialog::Instance()->IsOk())
                    {
                        std::string filePathName     = ImGuiFileDialog::Instance()->GetFilePathName();
                        // std::string file_ext = ImGuiFileDialog::Instance()->GetCurrentFilter(); // Useless if I can't set the file extension in the config of ImGuiFileDialog thing
                        // Log::trace("File extension: %s", file_ext.c_str());
                        live_fd_state.soundfont_path = ImGuiFileDialog::Instance()->GetCurrentPath();
                        FileDialogState::save(live_fd_state);

                        if(no_soundfont_duplicates)
                        {
                            if(!searchDuplicatedSoundfontItem(live_soundfont_list, filePathName))
                            {
                                // live_soundfont_list.emplace_back(SoundfontItem { filePathName, false }); // No one wants this to work like that
                                // Soo I decided to do it like this because it's better xd
                                if(live_soundfont_list.empty()) // Array is empty which means the first item must be added last (Or it crashes if inserted)
                                    live_soundfont_list.emplace_back(SoundfontItem { filePathName, false });
                                else
                                {
                                    // If the array isn't emtpy let the user add soundfont items in what order he wants
                                    size_t safe_index = std::min(static_cast<size_t>(selected_soundfont), live_soundfont_list.size() - 1);
                                    size_t insert_pos = safe_index + 1;
                                    live_soundfont_list.insert(live_soundfont_list.begin() + insert_pos, SoundfontItem { filePathName, false });
                                    selected_soundfont = static_cast<int>(insert_pos);
                                }
                                trigger_scroll_sf_list = true; // This does not work
                                SoundfontList::Save(live_soundfont_list);
                            }
                            else
                                // Todo: Add a message box to let the user know the soundfont already exists in the list
                                trigger_scroll_sf_list = true; // Scroll to the soundfont item that already exists
                        }
                        else
                        {
                            live_soundfont_list.emplace_back(SoundfontItem { filePathName, false });
                            trigger_scroll_sf_list = true; // Again. This does not work
                            SoundfontList::Save(live_soundfont_list);
                        }
                    }
                    ImGuiFileDialog::Instance()->Close();
                }

                ImGui::SameLine();

                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Add SoundFont item after the selected one");
                    ImGui::EndTooltip();
                }

                ImGui::SameLine();

                ImGui::BeginDisabled(live_soundfont_list.size() == 0);
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(179, 179, 179, 255));
                if(ImGui::Button(ICON_FA_TRASH_CAN, buton_sizes))
                {
                    live_soundfont_list.erase(live_soundfont_list.begin() + selected_soundfont);
                    SoundfontList::Save(live_soundfont_list);
                }
                ImGui::PopStyleColor();
                ImGui::EndDisabled();


                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Remove soundfont from the list");
                    ImGui::EndTooltip();
                }

                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 153, 0, 255));
                if(ImGui::Button(ICON_FA_CARET_UP, buton_sizes))
                {
                    moveSoundfont(selected_soundfont, -1);
                    selected_soundfont = selected_soundfont - 1;
                    SoundfontList::Save(live_soundfont_list);
                }
                ImGui::PopStyleColor();

                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Move up the selected soundfont");
                    ImGui::EndTooltip();
                }

                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 153, 0, 255));
                if(ImGui::Button(ICON_FA_CARET_DOWN, buton_sizes))
                {
                    moveSoundfont(selected_soundfont, +1);
                    selected_soundfont = selected_soundfont + 1;
                    SoundfontList::Save(live_soundfont_list);
                }
                ImGui::PopStyleColor();

                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Move down the selected soundfont");
                    ImGui::EndTooltip();
                }

                ImGui::SameLine();

                ImGui::BeginDisabled(live_soundfont_list.size() == 0);
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                if(ImGui::Button(ICON_FA_RECTANGLE_XMARK, buton_sizes))
                    ImGui::OpenPopup("Confirm clearance");
                ImGui::PopStyleColor();
                ImGui::EndDisabled();


                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Clear soundfont list");
                    ImGui::EndTooltip();
                }

                ImGui::SameLine();

                ImGui::BeginDisabled(live_soundfont_list.size() == 0);
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(51, 153, 255, 255));
                if(ImGui::Button(ICON_FA_CIRCLE_INFO, buton_sizes))
                {
                    current_file_info = FileHelpers::GetFileInfo(live_soundfont_list[selected_soundfont].label);

                    strncpy(file_name_buf, current_file_info.file_name.c_str(), sizeof(file_name_buf) - 1);
                    strncpy(file_size_buf, current_file_info.size.c_str(), sizeof(file_size_buf) - 1);
                    strncpy(last_mod_buf, current_file_info.last_mod.c_str(), sizeof(last_mod_buf) - 1);

                    file_info_window = true;
                    is_midi_info     = false;
                }
                ImGui::PopStyleColor();
                ImGui::EndDisabled();


                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Show file information");
                    ImGui::EndTooltip();
                }

                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

                if(ImGui::BeginPopupModal("Confirm clearance", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Are you sure you want to clear the soundfont list ?");
                    ImGui::Text("This action has no undo !!");

                    if(ImGui::Button("No", ImVec2(120.0f, 0)))
                        ImGui::CloseCurrentPopup();

                    ImGui::SameLine();

                    if(ImGui::Button("Yes", ImVec2(120.0f, 0)))
                    {
                        live_soundfont_list.clear();
                        SoundfontList::Save(live_soundfont_list);
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                RenderSoundfontList(live_soundfont_list, sf_search_text);

                ImGui::EndTabItem();
            }

            /*
                ▗▄▄▖▗▄▄▄▖▗▄▄▄▖▗▄▄▄▖▗▄▄▄▖▗▖  ▗▖ ▗▄▄▖ ▗▄▄▖
               ▐▌   ▐▌     █    █    █  ▐▛▚▖▐▌▐▌   ▐▌
                ▝▀▚▖▐▛▀▀▘  █    █    █  ▐▌ ▝▜▌▐▌▝▜▌ ▝▀▚▖
               ▗▄▄▞▘▐▙▄▄▖  █    █  ▗▄█▄▖▐▌  ▐▌▝▚▄▞▘▗▄▄▞▘



               ▗▄▄▄▖▗▄▖ ▗▄▄▖
                 █ ▐▌ ▐▌▐▌ ▐▌
                 █ ▐▛▀▜▌▐▛▀▚▖
                 █ ▐▌ ▐▌▐▙▄▞▘



            */

            std::ostringstream settings_tab_label;

            if(MidiList::missing_files || SoundfontList::missing_files)
                settings_tab_label << "Settings (" << ICON_FA_TRIANGLE_EXCLAMATION << ")";
            else
                settings_tab_label << "Settings";

            if(ImGui::BeginTabItem(settings_tab_label.str().c_str()))
            {
                if(ImGui::Button("Save"))
                    Config::Save(live_conf);

                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Save settings to configuration file");
                    ImGui::EndTooltip();
                }

                ImGui::SameLine();

                ImGui::Text("Settings marked with * require restart of the app\n\n");

                if(ImGui::BeginTabBar("sub-tabs", ImGuiTabBarFlags_None))
                {
                    if(ImGui::BeginTabItem("General"))
                    {
                        ImGui::Checkbox("No MIDI duplicates", &no_midi_duplicates);
                        if(ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Don't add the same previous MIDI to the list");
                            ImGui::EndTooltip();
                        }

                        ImGui::Checkbox("No Soundfont duplicates", &no_soundfont_duplicates);
                        if(ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Don't add the same Soundfont to the list");
                            ImGui::EndTooltip();
                        }

                        live_conf.no_midi_duplicates      = no_midi_duplicates;
                        live_conf.no_soundfont_duplicates = no_soundfont_duplicates;

                        if(MidiList::missing_files)
                        {
                            if(ImGui::CollapsingHeader("Missing MIDI Files"))
                            {
                                ImGui::Checkbox("Show full path", &show_full_path_lost_midis);
                                ImGui::BeginChild("##lostmidis", ImVec2(0, 190), true, ImGuiWindowFlags_HorizontalScrollbar);

                                for(size_t i = 0; i < MidiList::missing_files_list.size(); i++)
                                {
                                    std::string midi_filename;
                                    if(!show_full_path_lost_midis)
                                        midi_filename = FilenameOnly(MidiList::missing_files_list[i]);
                                    else
                                        midi_filename = MidiList::missing_files_list[i];

                                    size_t sel_idx    = static_cast<size_t>(selected_lost_midi);

                                    bool   isSelected = (i == sel_idx);

                                    if(ImGui::Selectable((midi_filename + "##" + std::to_string(i)).c_str(), isSelected))
                                        sel_idx = i;

                                    if(isSelected)
                                        ImGui::SetItemDefaultFocus();
                                }
                                ImGui::EndChild();
                            }
                        }

                        if(SoundfontList::missing_files)
                        {
                            if(ImGui::CollapsingHeader("Missing SoundFont Files"))
                            {
                                ImGui::Checkbox("Show full path", &show_full_path_lost_soundfonts);
                                ImGui::BeginChild("##lostsf", ImVec2(0, 190), true, ImGuiWindowFlags_HorizontalScrollbar);

                                for(size_t i = 0; i < SoundfontList::missing_files_list.size(); i++)
                                {
                                    std::string sf_filename;
                                    if(!show_full_path_lost_soundfonts)
                                        sf_filename = FilenameOnly(SoundfontList::missing_files_list[i]);
                                    else
                                        sf_filename = SoundfontList::missing_files_list[i];

                                    bool isSelected = (i == (size_t)selected_lost_soundfont);

                                    if(ImGui::Selectable((sf_filename + "##" + std::to_string(i)).c_str(), isSelected))
                                        selected_lost_soundfont = i;

                                    if(isSelected)
                                        ImGui::SetItemDefaultFocus();
                                }
                                ImGui::EndChild();
                            }
                        }

                        ImGui::EndTabItem();
                    }
                    /*
                        ▗▖  ▗▖▗▄▄▄▖ ▗▄▄▖▗▖ ▗▖ ▗▄▖ ▗▖
                        ▐▌  ▐▌  █  ▐▌   ▐▌ ▐▌▐▌ ▐▌▐▌
                        ▐▌  ▐▌  █   ▝▀▚▖▐▌ ▐▌▐▛▀▜▌▐▌
                         ▝▚▞▘ ▗▄█▄▖▗▄▄▞▘▝▚▄▞▘▐▌ ▐▌▐▙▄▄▖



                        ▗▄▄▄▖▗▄▖ ▗▄▄▖
                          █ ▐▌ ▐▌▐▌ ▐▌
                          █ ▐▛▀▜▌▐▛▀▚▖
                          █ ▐▌ ▐▌▐▙▄▞▘



                    */
                    if(ImGui::BeginTabItem("Visual"))
                    {
                        if(ImGui::Checkbox("Enable vsync", &vsync))
                        {
                            live_conf.vsync = vsync;

                            if(!SDL_SetRenderVSync(RenderWin->Ren, live_conf.vsync))
                                Log::error("Failed to set vsync (realtime)");
                        }

                        if(ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Enable or disable vertical synchronization for a smoother frame rate");
                            ImGui::EndTooltip();
                        }

                        ImGui::Text("FPS");
                        ImGui::SameLine();
                        ImGui::DragInt("##fps", &fps, 1, 10, 500);

                        live_conf.fps = fps;

                        ImGui::Separator();

                        ImGui::Checkbox("Overlap Remover *", &overlap_remover);
                        live_conf.OR = overlap_remover;
                        if(ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Removes duplicated note data reducing the visualization lag");
                            ImGui::EndTooltip();
                        }

                        ImGui::Text("Note Speed");

                        ImGui::SameLine();

                        ImGui::SliderInt("##NS", &live_note_speed, 100, 20000);

                        ImGuiStyle &style       = ImGui::GetStyle();
                        ImVec2      old_padding = style.FramePadding;
                        style.FramePadding      = ImVec2(style.FramePadding.y, style.FramePadding.y);

                        const f32 button_size   = ImGui::GetFrameHeight();
                        ImGui::SameLine();

                        if(ImGui::ButtonEx("+", ImVec2(button_size, button_size)))
                            live_note_speed++;

                        ImGui::SameLine();

                        if(ImGui::ButtonEx("-", ImVec2(button_size, button_size)))
                            live_note_speed--;

                        style.FramePadding = old_padding;

                        if(ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Change the background color of the main scene");
                            ImGui::EndTooltip();
                        }
                        liveColor = Frgba2Irgba(clear_color);

                        if(ImGui::CollapsingHeader("Note Colors"))
                        {
                            ImGui::Text("Pause to change colors");

                            ImGui::PushID("ccol_fd_btn");
                            if(ImGui::Button(ICON_FA_FOLDER_OPEN))
                            {
                                IGFD::FileDialogConfig config;
                                config.path  = live_fd_state.ccol_path;
                                config.flags = ImGuiFileDialogFlags_HideColumnType;
                                ImGuiFileDialog::Instance()->OpenDialog("CColFileFD", "Choose Channel Color Preset", ".ccol", config);
                            }
                            ImGui::PopID();
                            if(ImGui::BeginItemTooltip())
                            {
                                ImGui::Text("Choose a channel color preset");
                                ImGui::EndTooltip();
                            }

                            ConstrainWindowMove("Choose Channel Color Preset##CColFileFD");
                            ImGui::PushFont(FONT_icon_set);
                            if(ImGuiFileDialog::Instance()->Display("CColFileFD", 0, ImVec2(700, 500), ImVec2(FLT_MAX, FLT_MAX)))
                            {
                                if(ImGuiFileDialog::Instance()->IsOk())
                                {
                                    live_conf.last_ccol_file_path = ImGuiFileDialog::Instance()->GetFilePathName();
                                    live_fd_state.ccol_path       = ImGuiFileDialog::Instance()->GetCurrentPath();

                                    us_int *colors                = ChannelColors::getChannelColors(live_conf.last_ccol_file_path);

                                    live_conf.is_custom_ch_colors = true;

                                    // Safety is on the edge D:
                                    if(colors != nullptr)
                                        std::copy(colors, colors + 16, live_conf.channel_colors);
                                    else
                                        Log::error("Braadar what you were trying to do ?? That was never supposed to happen wha the heck ????");

                                    delete[] colors;

                                    FileDialogState::save(live_fd_state);
                                }

                                ImGuiFileDialog::Instance()->Close();
                            }
                            ImGui::PopFont();

                            ImGui::SameLine();

                            if(ImGui::Button("Reset"))
                            {
                                for(int i = 0; i < 16; i++)
                                {
                                    ui_chcolors[i]              = UIntToImVec4(default_settings.channel_colors[i]);
                                    live_conf.channel_colors[i] = default_settings.channel_colors[i];
                                }
                                live_conf.is_custom_ch_colors = false;
                            }
                            if(ImGui::BeginItemTooltip())
                            {
                                ImGui::Text("Reset the color order");
                                ImGui::EndTooltip();
                            }

                            for(int i = 0; i < 16; i++)
                            {
                                if(!is_defaultconfig)
                                    ui_chcolors[i] = UIntToImVec4(live_conf.channel_colors[i]);

                                temp_widget_id = "##Ch" + std::to_string(i);

                                ImGui::BeginDisabled(!Playback::is_paused);
                                if(ImGui::ColorEdit3(temp_widget_id.c_str(), (f32 *)&ui_chcolors[i], ImGuiColorEditFlags_NoInputs))
                                    live_conf.is_custom_ch_colors = true;
                                ImGui::EndDisabled();

                                ImGui::SameLine();

                                live_conf.channel_colors[i] = ImVec4ToUInt(ui_chcolors[i]);
                            }
                            ImGui::Text("\n");

                            ImGui::Checkbox("Loop colors ", &loop_colors);
                            live_conf.loop_colors = loop_colors;

                            if(ImGui::BeginItemTooltip())
                            {
                                ImGui::Text("The colors will be applied if track count exceeds 16\nIf no loop colors are enabled, random colors will be generated from track 17");
                                ImGui::EndTooltip();
                            }
                        }

                        if(ImGui::CollapsingHeader("Background"))
                        {
                            // There's some flickering issue here
                            ImGui::Text("Background Color");
                            clear_color = ImVec4(live_conf.bg_R / 255.0f, live_conf.bg_G / 255.0f, live_conf.bg_B / 255.0f, live_conf.bg_A / 255.0f);

                            ImGui::ColorEdit3("##H", (f32 *)&clear_color);

                            ImGui::Checkbox("Vertical Lines", &vertical_lines);
                            live_conf.draw_vertical_lines = vertical_lines;

                            ImGui::Checkbox("Measure Lines", &draw_measure_lines);
                            live_conf.draw_measure_lines = draw_measure_lines;

                            if(ImGui::BeginItemTooltip())
                            {
                                ImGui::Text("Show the vertical lines in between the white keys");
                                ImGui::EndTooltip();
                            }

                            if(ImGui::Checkbox("Background image", &background_image))
                                RenderWin->LoadBackgroundImage(live_conf.background_image_path);

                            live_conf.background_image = background_image;

                            ImGui::SameLine();

                            if(ImGui::Button(ICON_FA_FOLDER_OPEN))
                            {
                                IGFD::FileDialogConfig config;
                                config.path  = live_fd_state.bg_image_path;
                                config.flags = ImGuiFileDialogFlags_HideColumnType;
                                ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".png", internal_style.Colors[ImGuiCol_Text], ICON_FA_FILE_IMAGE);
                                ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".jpg", internal_style.Colors[ImGuiCol_Text], ICON_FA_FILE_IMAGE);
                                ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".jpeg", internal_style.Colors[ImGuiCol_Text], ICON_FA_FILE_IMAGE);
                                ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".webp", internal_style.Colors[ImGuiCol_Text], ICON_FA_FILE_IMAGE);
                                ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".bmp", internal_style.Colors[ImGuiCol_Text], ICON_FA_FILE_IMAGE);
                                ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".svg", internal_style.Colors[ImGuiCol_Text], ICON_FA_FILE_IMAGE);

                                ImGuiFileDialog::Instance()->OpenDialog("ImageFileFD", "Choose Image File", ".png,.jpg,.jpeg,.webp,.bmp,.svg", config);

                                ImGuiFileDialog::Instance()->AddPlacesGroup("Places", 0, false, true);
                                // Then get the pointer
                                auto places_ptr = ImGuiFileDialog::Instance()->GetPlacesGroupPtr("Places");
                                if(places_ptr)
                                {
            #ifdef PLATFORM_ANDROID
                                    IGFD::FileStyle f_style_home;
                                    f_style_home.color = internal_style.Colors[ImGuiCol_Text];
                                    f_style_home.icon  = ICON_FA_HOUSE;
            
                                    places_ptr->AddPlace("Home", "/storage/emulated/0", false, f_style_home);
            #else
                                    IGFD::FileStyle f_style_home;
                                    f_style_home.color = internal_style.Colors[ImGuiCol_Text];
                                    f_style_home.icon  = ICON_FA_HOUSE;
            
                                    IGFD::FileStyle f_style_downloads;
                                    f_style_downloads.color = internal_style.Colors[ImGuiCol_Text];
                                    f_style_downloads.icon  = ICON_FA_DOWNLOAD;
            
                                    places_ptr->AddPlace("Home", "/home/andre/Desktop", false, f_style_home);
                                    places_ptr->AddPlace("Downloads", "/home/andre/Downloads", false, f_style_downloads);
            #endif
                                }
                            }
                            if(ImGui::BeginItemTooltip())
                            {
                                ImGui::Text("Load a background image");
                                ImGui::EndTooltip();
                            }

                            ImGui::PushFont(FONT_icon_set);
                            ConstrainWindowMove("Choose Image File##ImageFileFD");
                            if(ImGuiFileDialog::Instance()->Display("ImageFileFD", 0, ImVec2(700, 500), ImVec2(FLT_MAX, FLT_MAX)))
                            {
                                if(ImGuiFileDialog::Instance()->IsOk())
                                {
                                    live_conf.background_image_path = ImGuiFileDialog::Instance()->GetFilePathName();
                                    live_fd_state.bg_image_path     = ImGuiFileDialog::Instance()->GetCurrentPath();
                                    FileDialogState::save(live_fd_state);

                                    RenderWin->LoadBackgroundImage(live_conf.background_image_path);
                                }

                                ImGuiFileDialog::Instance()->Close();
                            }
                            ImGui::PopFont();

                            ImGui::Text("Image file: %s", FilenameOnly(live_conf.background_image_path).c_str());
                        }

                        if(ImGui::CollapsingHeader("UI Theme"))
                        {
                            if(ImGui::Checkbox("Enable custom UI theme", &ui_theming))
                            {
                                live_conf.custom_ui_theme = ui_theming;
                                if(!ui_theming)
                                    // SetMoonlightTheme();
                                    SetBuiltinTheme(builtin_ui_theme_idx);
                                else if(std::filesystem::exists(live_conf.ui_theme_file_path))
                                {
                                    ImGui::LoadStyleFrom(live_conf.ui_theme_file_path.c_str());
                                    text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                                }
                                // Warn the user if the theme file is missing
                                else
                                    // SetMoonlightTheme();
                                    SetBuiltinTheme(builtin_ui_theme_idx);
                            }

                            ImGui::SameLine();

                            ImGui::BeginDisabled(!live_conf.custom_ui_theme);
                            ImGui::PushID("uiThemeId");
                            if(ImGui::Button(ICON_FA_FOLDER_OPEN))
                            {
                                IGFD::FileDialogConfig config;
                                config.path  = live_fd_state.ui_theme_path;
                                config.flags = ImGuiFileDialogFlags_HideColumnType;
                                ImGuiFileDialog::Instance()->OpenDialog("UiThemeFD", "Choose a Custom UI Theme", ".imst", config);
                            }
                            ImGui::PopID();
                            if(ImGui::BeginItemTooltip())
                            {
                                ImGui::Text("Choose a UI Theme File");
                                ImGui::EndTooltip();
                            }

                            ImGui::PushFont(FONT_icon_set);
                            ConstrainWindowMove("Choose a Custom UI Theme##UiThemeFD");
                            if(ImGuiFileDialog::Instance()->Display("UiThemeFD", 0, ImVec2(700, 500), ImVec2(FLT_MAX, FLT_MAX)))
                            {
                                if(ImGuiFileDialog::Instance()->IsOk())
                                {
                                    live_conf.ui_theme_file_path = ImGuiFileDialog::Instance()->GetFilePathName();
                                    live_fd_state.ui_theme_path  = ImGuiFileDialog::Instance()->GetCurrentPath();
                                    FileDialogState::save(live_fd_state);
                                    ImGui::LoadStyleFrom(live_conf.ui_theme_file_path.c_str());
                                }

                                ImGuiFileDialog::Instance()->Close();
                            }
                            ImGui::PopFont();
                            ImGui::EndDisabled();

                            ImGui::Text("Current theme: %s", live_conf.ui_theme_file_path.c_str());

                            live_conf.builtin_ui_theme      = !ui_theming;

                            const char *combo_preview_value = builtin_ui_theme_names[builtin_ui_theme_idx];
                            ImGui::BeginDisabled(ui_theming);
                            if(ImGui::BeginCombo("Builtin themes", combo_preview_value))
                            {
                                for(int n = 0; n < IM_COUNTOF(builtin_ui_theme_names); n++)
                                {
                                    const bool is_selected = (builtin_ui_theme_idx == n);
                                    if(ImGui::Selectable(builtin_ui_theme_names[n], is_selected))
                                    {
                                        builtin_ui_theme_idx = n;
                                        SetBuiltinTheme(builtin_ui_theme_idx);
                                        live_conf.builtin_ui_theme_idx = builtin_ui_theme_idx;
                                    }

                                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                    if(is_selected)
                                        ImGui::SetItemDefaultFocus();
                                }
                                ImGui::EndCombo();
                            }
                            ImGui::EndDisabled();
                        }

                        ImGui::EndTabItem();
                    }
                    /*
                        ▗▄▖ ▗▖ ▗▖▗▄▄▄ ▗▄▄▄▖ ▗▄▖
                       ▐▌ ▐▌▐▌ ▐▌▐▌  █  █  ▐▌ ▐▌
                       ▐▛▀▜▌▐▌ ▐▌▐▌  █  █  ▐▌ ▐▌
                       ▐▌ ▐▌▝▚▄▞▘▐▙▄▄▀▗▄█▄▖▝▚▄▞▘



                       ▗▄▄▄▖▗▄▖ ▗▄▄▖
                         █ ▐▌ ▐▌▐▌ ▐▌
                         █ ▐▛▀▜▌▐▛▀▚▖
                         █ ▐▌ ▐▌▐▙▄▞▘



                    */
                    if(ImGui::BeginTabItem("Audio"))
                    {
                        live_conf.audio_device_index = UI::current_audio_dev;
                        ShowAudioDeviceList(availableAudioDevices);
                        ImGui::Text("\n");
                        ImGui::Text("Voice Count");
                        // Store the previous value to detect changes
                        static int prev_voice_count = live_conf.bass_voice_count;

                        // Input widget for voice count
                        if(ImGui::InputInt("##LOL", &live_conf.bass_voice_count))
                        {
                            // Ensure value is within reasonable limits
                            if(live_conf.bass_voice_count < 1)
                                live_conf.bass_voice_count = 1;

                            if(live_conf.bass_voice_count > 5000)
                                live_conf.bass_voice_count = 5000;

                            // Apply the change in real-time if the value has changed
                            if(prev_voice_count != live_conf.bass_voice_count)
                            {
                                Playback::updateBassVoiceCount(live_conf.bass_voice_count);
                                prev_voice_count = live_conf.bass_voice_count;
                            }
                        }

                        ImGui::Text("Effects");
                        if(ImGui::CollapsingHeader("Velocity Filter *"))
                        {
                            ImGui::Checkbox("Enabled", &velocity_filter);
                            live_conf.vel_filter = velocity_filter;

                            ImGui::BeginDisabled(!live_conf.vel_filter); // Disable widgets if isDisabled is true
                            {
                                ImGui::SliderInt("Min Velocity", &min_velocity, 0, 127);
                                ImGui::SliderInt("Max velocity", &max_velocity, 0, 127);
                            }
                            ImGui::EndDisabled();

                            live_conf.vel_min = min_velocity;
                            live_conf.vel_max = max_velocity;
                        }
                        ImGui::EndTabItem();
                    }
                    /*
                        ▗▖    ▗▄▖  ▗▄▄▖ ▗▄▄▖
                        ▐▌   ▐▌ ▐▌▐▌   ▐▌
                        ▐▌   ▐▌ ▐▌▐▌▝▜▌ ▝▀▚▖
                        ▐▙▄▄▖▝▚▄▞▘▝▚▄▞▘▗▄▄▞▘



                        ▗▄▄▄▖▗▄▖ ▗▄▄▖
                          █ ▐▌ ▐▌▐▌ ▐▌
                          █ ▐▛▀▜▌▐▛▀▚▖
                          █ ▐▌ ▐▌▐▙▄▞▘



                    */
                    if(ImGui::BeginTabItem("Logs"))
                    {
                        ImGui::BeginDisabled(!internal_logging);
                        if(ImGui::Button("Open Log viewer"))
                            log_buffer_window = true;

                        ImGui::EndDisabled();

                        ImGui::Checkbox("Log to internal buffer", &internal_logging);
                        live_conf.internal_log_buffer = internal_logging;

                        ImGui::Checkbox("Log to file*", &log_to_file);
                        live_conf.log_to_file = log_to_file;

                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                    ImGui::Text("\n");
                }

                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Set how many notes can be played on specific instruments (changes apply immediately)");
                    ImGui::EndTooltip();
                }

                // Keeping the background color updated
                live_conf.bg_R       = liveColor.r;
                live_conf.bg_G       = liveColor.g;
                live_conf.bg_B       = liveColor.b;
                live_conf.bg_A       = liveColor.a;
                live_conf.note_speed = live_note_speed;

                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("About"))
            {
                ImGui::BeginChild("ScrollRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
#ifdef PLATFORM_ANDROID
                {
                    static KineticState ks_about_y;
                    static KineticState ks_about_x;
                    bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
                    bool dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
                    ImGui::SetScrollY((f32)ks_about_y.Update(ImGui::GetScrollY(), io.MouseDelta.y, io.DeltaTime, hovered, dragging));
                    ImGui::SetScrollX((f32)ks_about_x.Update(ImGui::GetScrollX(), io.MouseDelta.x, io.DeltaTime, hovered, dragging));
                }
#endif
                ImGui::Text("A clone of the original Piano From Above for mobile based on Qishipai's midi processing library.");
                ImGui::Text("Authors:");
                ImGui::Text("NVirsual: Qishipai");
                ImGui::Text("Piano From Above imitation: Tweak1600");
                ImGui::Text("Improved by:");
                ImGui::Text("Kiptunor");
                ImGui::Text("Hexagon-Midis\n\n");
                ImGui::Text("Icon Made by Zeal");
                ImGui::Text("Powered by: SDL3, Imgui, bass and bass plugins");
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
        internal_style = ImGui::GetStyle();
    } // Main window

    /*
        ▗▄▄▄▖▗▄▄▄▖▗▖   ▗▄▄▄▖    ▗▄▄▄▖▗▖  ▗▖▗▄▄▄▖ ▗▄▖
        ▐▌     █  ▐▌   ▐▌         █  ▐▛▚▖▐▌▐▌   ▐▌ ▐▌
        ▐▛▀▀▘  █  ▐▌   ▐▛▀▀▘      █  ▐▌ ▝▜▌▐▛▀▀▘▐▌ ▐▌
        ▐▌   ▗▄█▄▖▐▙▄▄▖▐▙▄▄▖    ▗▄█▄▖▐▌  ▐▌▐▌   ▝▚▄▞▘


        ▗▖ ▗▖▗▄▄▄▖▗▖  ▗▖▗▄▄▄  ▗▄▖ ▗▖ ▗▖
        ▐▌ ▐▌  █  ▐▛▚▖▐▌▐▌  █▐▌ ▐▌▐▌ ▐▌
        ▐▌ ▐▌  █  ▐▌ ▝▜▌▐▌  █▐▌ ▐▌▐▌ ▐▌
        ▐▙█▟▌▗▄█▄▖▐▌  ▐▌▐▙▄▄▀▝▚▄▞▘▐▙█▟▌



    */
    ConstrainWindowMove("File Information");

    if(file_info_window)
    {
        ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(center, ImGuiCond_Once, ImVec2(0.5f, 0.5f)); // Pivot 0.5 = center
#ifndef PLATFORM_ANDROID
        ImGui::SetNextWindowSizeConstraints(ImVec2(500, 380), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin("File Information", &file_info_window);
#else // Setting up a different ui layout for mobile users
        ImGui::SetNextWindowSize(ImVec2(1290.0f, 600.0f));
        ImGui::Begin("File Information", &file_info_window, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_HorizontalScrollbar);
#endif


#ifdef PLATFORM_ANDROID
        {
            static KineticState ks_fi_y;
            static KineticState ks_fi_x;
            bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            bool dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
            ImGui::SetScrollY((f32)ks_fi_y.Update(ImGui::GetScrollY(), io.MouseDelta.y, io.DeltaTime, hovered, dragging));
            ImGui::SetScrollX((f32)ks_fi_x.Update(ImGui::GetScrollX(), io.MouseDelta.x, io.DeltaTime, hovered, dragging));
        }
#endif

        ImGui::Text("File:                         ");
        ImGui::SameLine();
        ImGui::InputText("##nu", file_name_buf, sizeof(file_name_buf));
        ImGui::SameLine();
        ImGui::PushID("cp1");
        if(ImGui::Button(ICON_FA_COPY))
            SDL_SetClipboardText(file_name_buf);
        ImGui::PopID();

        ImGui::Text("File Size:                 ");
        ImGui::SameLine();
        ImGui::InputText("##a", file_size_buf, sizeof(file_size_buf), ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        ImGui::PushID("cp2");
        if(ImGui::Button(ICON_FA_COPY))
            SDL_SetClipboardText(file_size_buf);
        ImGui::PopID();

        ImGui::Text("Last Modification: ");
        ImGui::SameLine();
        ImGui::InputText("##heh", last_mod_buf, sizeof(last_mod_buf), ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        ImGui::PushID("cp3");
        if(ImGui::Button(ICON_FA_COPY))
            SDL_SetClipboardText(last_mod_buf);
        ImGui::PopID();

        std::ostringstream basic_file_info;
        basic_file_info << "File: " << file_name_buf << "\n";
        basic_file_info << "Size: " << file_size_buf << "\n";
        basic_file_info << "Last Modification: " << last_mod_buf << "\n";

        if(ImGui::Button("Copy basic info"))
            SDL_SetClipboardText(basic_file_info.str().c_str());

        if(ImGui::BeginItemTooltip())
        {
            ImGui::Text("Copy basic file info to clipboard (The file info above)");
            ImGui::EndTooltip();
        }

        if(is_midi_info)
        {
            if(ImGui::Button("More MIDI Information (for professionals)"))
                show_midi_details = !show_midi_details;

            if(show_midi_details && cached_midi_info.success)
            {
                ImGui::Text("PPQN: %d", cached_midi_info.ppqn);
                if(cached_midi_info.bpm > 0.0)
                    ImGui::Text("BPM: %.2f", cached_midi_info.bpm);
                else
                    ImGui::Text("BPM: N/A");
                if(cached_midi_info.timeSigNum > 0)
                    ImGui::Text("Time Sig: %d/%d", cached_midi_info.timeSigNum, cached_midi_info.timeSigDen);
                else
                    ImGui::Text("Time Sig: N/A");
            }
        }

        ImGui::End();
    } // File info window


    /*
        ▗▖    ▗▄▖  ▗▄▄▖ ▗▄▄▖
        ▐▌   ▐▌ ▐▌▐▌   ▐▌
        ▐▌   ▐▌ ▐▌▐▌▝▜▌ ▝▀▚▖
        ▐▙▄▄▖▝▚▄▞▘▝▚▄▞▘▗▄▄▞▘



        ▗▖ ▗▖▗▄▄▄▖▗▖  ▗▖▗▄▄▄  ▗▄▖ ▗▖ ▗▖
        ▐▌ ▐▌  █  ▐▛▚▖▐▌▐▌  █▐▌ ▐▌▐▌ ▐▌
        ▐▌ ▐▌  █  ▐▌ ▝▜▌▐▌  █▐▌ ▐▌▐▌ ▐▌
        ▐▙█▟▌▗▄█▄▖▐▌  ▐▌▐▙▄▄▀▝▚▄▞▘▐▙█▟▌



    */


    if(log_buffer_window)
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(500, 380), ImVec2(FLT_MAX, FLT_MAX));
        ConstrainWindowMove("Logs");
        ImGui::Begin("Logs", &log_buffer_window);

        ImGui::BeginDisabled(log_buffer_cleared);
        if(ImGui::Button(ICON_FA_COPY))
            SDL_SetClipboardText(Log::log_buffer[selected_log_line].c_str());

        ImGui::EndDisabled();

        if(ImGui::BeginItemTooltip())
        {
            ImGui::Text("Copy selected log lines to clipboard");
            ImGui::EndTooltip();
        }

        ImGui::SameLine();


        if(ImGui::Button(ICON_FA_CIRCLE_XMARK))
        {
            Log::log_buffer.clear();
            log_buffer_cleared = true;
        }

        if(ImGui::BeginItemTooltip())
        {
            ImGui::Text("Clear log buffer");
            ImGui::EndTooltip();
        }


        ImGui::BeginChild("##logs_text", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        bool scroll_to_bottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();

        /*
        #ifdef PLATFORM_ANDROID
                // Single finger drag scroll (More comfortable than the scrollbar on mobile)
                if(ImGui::IsItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseDelta.y);
                    //ImGuiWindow* child = ImGui::GetCurrentWindow();
                    //child->Scroll.y -= ImGui::GetIO().MouseDelta.y;
                }
        #endif
        */

#ifdef PLATFORM_ANDROID
        {
            static KineticState ks_log_y;
            static KineticState ks_log_x;
            bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            bool dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
            ImGui::SetScrollY((f32)ks_log_y.Update(ImGui::GetScrollY(), io.MouseDelta.y, io.DeltaTime, hovered, dragging));
            ImGui::SetScrollX((f32)ks_log_x.Update(ImGui::GetScrollX(), io.MouseDelta.x, io.DeltaTime, hovered, dragging));
        }
#endif

        if(Log::log_buffer.size() != 0)
            log_buffer_cleared = false;

        size_t sel_idx = static_cast<size_t>(selected_log_line);
        for(size_t i = 0; i < Log::log_buffer.size(); ++i)
        {
            bool isSelected = (i == sel_idx);

            if(ImGui::Selectable((Log::log_buffer[i] + "##" + std::to_string(i)).c_str(), isSelected))
                sel_idx = i;

            selected_log_line = (int)sel_idx;

            if(isSelected)
                ImGui::SetItemDefaultFocus();
        }
        if(scroll_to_bottom)
            ImGui::SetScrollHereY(1.0f);


        ImGui::EndChild();
        ImGui::End();
    }

    ImGui::End();

    // Rendering
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), r);
}