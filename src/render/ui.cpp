#include <string>
#include <filesystem>
#include <iostream>



#include "ui.h"
#include "render.h"
#include "../config/soundfont_list.h"
#include "../config/midi_list.h"
#include "../audio/playback.h"
#include "../logger.h"
#include "../../assets/FA6FreeSolidFontData.h"
#include "../../assets/IconsFontAwesome6.h"
#include "../file_helpers.h"


#include <imgui.h>
#include <imgui_internal.h>
#include <backend_render/imgui_impl_sdl3.h>
#include <backend_render/imgui_impl_sdlrenderer3.h>
#include <file_dlg/ImGuiFileDialog.h>





// - - - - [Internal variables] - - - -
bool file_info_window = false;
bool allow_audio_dev_ssave;
static char midi_search[128];
static char sf_search[128];
static std::string midi_search_text;
static std::string sf_search_text;
static std::string img_filename;
static std::string sf_filename;
static std::string midi_filename;
static char soundfons_path_entry[1024];
int selected_midi_path_entry;
int selected_soundfont_path_etry;
int selected_img_path_entry;
int selected_soundfont = 0;
ImFont* FONT_icon_set;
std::string temp_widget_id;
std::ostringstream file_info_fields;
std::vector <std::string> current_soundfonts;
FileHelpers::FileInfo current_file_info;


// UI/Widget variables
bool UI::show_demo_window = false;
bool UI::main_gui_window = false;
int  UI::live_note_speed = 6000;
int  UI::selIndex = 0;
bool UI::velocity_filter = true;
bool UI::loop_colors = false;
bool UI::overlap_remover = true;
bool UI::use_bg_image = false;
bool UI::no_midi_duplicates;
bool UI::no_soundfont_duplicates;
bool UI::vsync;
bool UI::vertical_lines;
bool UI::use_default_media_paths = true;
bool UI::background_image;
int  UI::min_velocity;
int  UI::max_velocity;
std::string UI::last_midi_path;
std::string UI::last_midi_file;
std::string UI::last_sf_path;
ImVec4 UI::clear_color;
UI::RGBAint UI::liveColor;
ImVec4 UI::ui_chcolors[16];
int UI::current_audio_dev;
std::vector<std::string> UI::soundfont_paths;
std::vector<std::string> UI::prev_images;






void SetupIconFonts()
{
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.FontDataOwnedByAtlas = false;
    float size = FONT_AWESOME_ICON_SIZE;
    
    icons_config.GlyphMaxAdvanceX = std::numeric_limits<float>::max();
    icons_config.RasterizerMultiply = 1.0f;
    icons_config.OversampleH = 2;
    icons_config.OversampleV = 1;
    
    icons_config.GlyphRanges = icons_ranges;
    
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromMemoryCompressedTTF((void*)fa_solid_900_compressed_data, fa_solid_900_compressed_size, size, &icons_config, icons_ranges);
}

void UI::SetDefaultTheme()
{
    /*
    Theme: Moonlight
    Author: deathsu/madam-herta
    Original source: https://github.com/Madam-Herta/Moonlight
    */
    ImGuiStyle& style = ImGui::GetStyle();
	
	style.Alpha                    = 1.0f;
	style.DisabledAlpha            = 1.0f;
	style.WindowPadding            = ImVec2(12.0f, 12.0f);
	style.WindowRounding           = 11.5f;
	style.WindowBorderSize         = 0.0f;
	style.WindowMinSize            = ImVec2(20.0f, 20.0f);
	style.WindowTitleAlign         = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.ChildRounding            = 0.0f;
	style.ChildBorderSize          = 1.0f;
	style.PopupRounding            = 0.0f;
	style.PopupBorderSize          = 1.0f;
	style.FramePadding             = ImVec2(20.0f, 3.400000095367432f);
	style.FrameRounding            = 11.89999961853027f;
	style.FrameBorderSize          = 0.0f;
	style.ItemSpacing              = ImVec2(4.300000190734863f, 5.5f);
	style.ItemInnerSpacing         = ImVec2(7.099999904632568f, 1.799999952316284f);
	style.CellPadding              = ImVec2(12.10000038146973f, 9.199999809265137f);
	style.IndentSpacing            = 0.0f;
	style.ColumnsMinSpacing        = 4.900000095367432f;
	
	// Different size for the scrollbar so user can actually grab it lol
#ifndef PLATFORM_ANDROID
	style.ScrollbarSize            = 20.60000038146973f;
#else
	style.ScrollbarSize            = 48.60000038146973f;
#endif

	style.ScrollbarRounding        = 15.89999961853027f;
#ifndef PLATFORM_ANDROID
	style.GrabMinSize              = 12.700000047683716f;
#else
	style.GrabMinSize              = 40.700000047683716f;
#endif

	style.GrabRounding             = 8.0f;                // Modified
	style.TabRounding              = 8.89999961853027f;
	style.TabBorderSize            = 0.0f;
	// style.TabMinWidthForCloseButton = 0.0f; // This seems to be deprecated in Imgui v1.91.9b
	style.ColorButtonPosition      = ImGuiDir_Right;
	style.ButtonTextAlign          = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign      = ImVec2(0.0f, 0.0f);
	
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
	style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.9356134533882141f, 0.9356129765510559f, 0.9356223344802856f, 1.0f);
	style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavHighlight]          = ImVec4(0.266094446182251f, 0.2890366911888123f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
	style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
}

void UI::Setup(SDL_Window *w, SDL_Renderer *r)
{
    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = IMGUI_INI_FILE_PATH;                 // Custom path for imgui ini
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    
    // Setup ImGui style
    ImGui::StyleColorsDark();
    SetDefaultTheme(); // Setting a nice looking GUI :3
    
    //Metrophobic-Regular.ttf
    //Font suggested by Nerdly
#ifdef PLATFORM_ANDROID
    //std::string ui_font_file = NVFileUtils::GetFilePathA("ui_font.ttf", "rb");
    
    io.Fonts->AddFontFromFileTTF("ui_font.ttf", 38.0f);
#else
  io.Fonts->AddFontFromFileTTF("ui_font.ttf", 22.0f);
  //FONT_icon_set = io.Fonts->AddFontFromFileTTF("fontello.ttf", 14.0f);
  //io.Fonts->AddFontFromFileTTF("ui_font.ttf", 19.0f);
#endif

    SetupIconFonts();
    
    // Setup Platform/Renderer backends
    //ImGui_ImplSDL3_InitForSDLRenderer(w, r);
    ImGui_ImplSDLRenderer3_Init(r);
    ImGui_ImplSDL3_InitForVulkan(w);
}






/*
- - - - [UI Functions] - - - -
*/

UI::RGBAint UI::Frgba2Irgba(ImVec4& col)
{
    RGBAint res;
    res.r = static_cast<int>(col.x * 255.0f);
    res.g = static_cast<int>(col.y * 255.0f);
    res.b = static_cast<int>(col.z * 255.0f);
    res.a = static_cast<int>(col.w * 255.0f);
    return res;
}

ImVec4 UI::Irgba2ImVec4(int r, int g, int b, int a)
{
    return ImVec4(
        r / 255.0f,
        g / 255.0f,
        b / 255.0f,
        a / 255.0f
    );
}


/*
Functions for internal use only
*/


// So many of you wanted this
// And I can relate to such a problem

// Filter only the filenames before showing them on the midi and soundfont lists or it will cause a godamn stupid chaos
std::string FilenameOnly(const std::string& path)
{
    size_t slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? path : path.substr(slash + 1);
}

bool searchDuplicatedMidiItem(const std::vector<std::string>& items, const std::string& item)
{
    for(const auto& i : items)
        if(i == item)
            return true;
    return false;
}

bool searchDuplicatedSoundfontItem(const std::vector<UI::SoundfontItem>& items, const std::string& item)
{
    for(const auto& i : items)
        if(i.label == item)
            return true;
    return false;
}

void RenderMidiList(const std::vector<std::string>& items, int& selectedIndex, std::string find_item)
{
    ImGui::BeginChild("##midils", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    
    for(int i = 0; i < items.size(); ++i)
    {
        std::string midi_filename = FilenameOnly(items[i]);
    
        // Filter check (case-insensitive optional)
        if(!find_item.empty() && midi_filename.find(find_item) == std::string::npos)
            continue;
    
        bool isSelected = (i == selectedIndex);
    
        if(ImGui::Selectable((midi_filename + "##" + std::to_string(i)).c_str(), isSelected))
        {
            selectedIndex = i;
        }
    
        if(isSelected)
            ImGui::SetItemDefaultFocus();
    }
    
        ImGui::EndChild();
}

void RenderSoundfontList(std::vector<UI::SoundfontItem>& items, std::string find_item)
{
    ImGui::BeginChild("##sfls", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    
    bool soundfont_changed = false;
    
    for(int i = 0; i < items.size(); ++i)
    {
        std::string sf_filename = FilenameOnly(items[i].label);
    
        // Filter check
        if(!find_item.empty() && sf_filename.find(find_item) == std::string::npos)
            continue;
            
        // Store previous checkbox state
        bool previous_state = items[i].checked;
        
        std::string temp = "##" + std::to_string(i);
        if(ImGui::Selectable(temp.c_str(), selected_soundfont == i, ImGuiSelectableFlags_AllowOverlap, ImVec2(0, 28)))
            selected_soundfont = i;
        
        ImGui::SameLine();
    
        // Unique ID to avoid conflicts
        ImGui::Checkbox((sf_filename + "##" + std::to_string(i)).c_str(), &items[i].checked);
        
        // Check if state changed
        if(previous_state != items[i].checked)
        {
            soundfont_changed = true;
        }
    }
    
    ImGui::EndChild();
    
    // If any soundfont selection changed, update the checked_soundfonts list
    // and reload soundfonts immediately
    if(soundfont_changed)
    {
        checked_soundfonts = UI::GetCheckedSoundfonts(items);
        SoundfontList::Save(live_soundfont_list);
        Playback::ReloadSoundfonts();
    }
}

std::vector<std::string> UI::GetCheckedSoundfonts(const std::vector<SoundfontItem>& items)
{
    std::vector<std::string> checkedItems;
    
    for(const auto& item : items)
    {
        if(item.checked)
        {
            checkedItems.push_back(item.label);
        }
    }
    
    return checkedItems;
}



void RenderSoundfontsPathsList(const std::vector<std::string>& items, int& selectedIndex)
{
    ImGui::BeginChild("##soundfontspathls", ImVec2(0, 190), true, ImGuiWindowFlags_HorizontalScrollbar);
    
    for(int i = 0; i < items.size(); ++i)
    {
        bool isSelected = (i == selectedIndex);
    
        if(ImGui::Selectable((items[i] + "##" + std::to_string(i)).c_str(), isSelected))
        {
            selectedIndex = i;
        }
    
        if(isSelected)
            ImGui::SetItemDefaultFocus();
    }
    
    ImGui::EndChild();
}

void ShowAudioDeviceList(const std::vector<Playback::AudioDevice>& audioDevices)
{
    std::vector<const char*> deviceNames;
    for(const auto& device : audioDevices)
    {
        deviceNames.push_back(device.name);
    }
    
    // Ensure current_audio_dev is valid
    if(deviceNames.empty())
    {
        UI::current_audio_dev = 0; // Reset to 0 if there are no devices
        allow_audio_dev_ssave = false;
    }
    else if(UI::current_audio_dev >= deviceNames.size())
    {
        allow_audio_dev_ssave = true;
        UI::current_audio_dev = 0; // Reset to the first device if the index is out of bounds
    }
    
    // Display the combo box
    const char* currentDeviceName = (deviceNames.empty() ? "No devices available" : deviceNames[UI::current_audio_dev]);
    
    if(ImGui::BeginCombo("Audio Devices *", currentDeviceName))
    {
        if(deviceNames.empty())
        {
            // Render a placeholder item when no devices are available
            ImGui::Selectable("No devices available", false);
        }
        else
        {
            for(int i = 0; i < deviceNames.size(); i++)
            {
                // Check if this item is selected
                bool isSelected = (UI::current_audio_dev == i);
    
                // Add the item to the combo box
                if(ImGui::Selectable(deviceNames[i], isSelected))
                {
                    // Update the current index if the user selects this item
                    UI::current_audio_dev = i;
                }
    
                // Set the initial focus when opening the combo box
                if(isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
        }
        ImGui::EndCombo();
    }
}

unsigned int UI::ImVec4ToUInt(const ImVec4& color)
{
    unsigned int r = static_cast<unsigned int>(color.x * 255.0f);
    unsigned int g = static_cast<unsigned int>(color.y * 255.0f);
    unsigned int b = static_cast<unsigned int>(color.z * 255.0f);
    return (r << 16) | (g << 8) | b;
}

ImVec4 UI::UIntToImVec4(unsigned int rgb)
{
    float alpha = 1.0f;
    float r = ((rgb >> 16) & 0xFF) / 255.0f;
    float g = ((rgb >> 8)  & 0xFF) / 255.0f;
    float b = (rgb         & 0xFF) / 255.0f;
    return ImVec4(r, g, b, alpha);
}

void ConstrainWindowMove(const char* windowName)
{
    ImGuiContext& g = *GImGui;
    
    ImGuiWindow* win = ImGui::FindWindowByName(windowName);
    if(g.MovingWindow != nullptr && g.MovingWindow == win)
    {
        ImVec2 viewportPos = g.MovingWindow->Viewport->Pos;
        ImVec2 viewportSize = g.MovingWindow->Viewport->Size;
        ImVec2 windowSize = g.MovingWindow->Size;
        
        ImVec2 minPos = viewportPos;
        ImVec2 maxPos;
        maxPos.x = viewportPos.x + viewportSize.x - windowSize.x;
        maxPos.y = viewportPos.y + viewportSize.y - windowSize.y;
        
        ImVec2 pos = g.MovingWindow->Pos;
        
        pos.x = std::clamp(pos.x, minPos.x, maxPos.x);
        pos.y = std::clamp(pos.y, minPos.y, maxPos.y);
        
        if(pos.x != g.MovingWindow->Pos.x || pos.y != g.MovingWindow->Pos.y)
        {
            g.MovingWindow->Pos = pos;
        }
    }
}

void UI::Render(SDL_Renderer *r)
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    
    ImGuiIO& io = ImGui::GetIO();
    
    
    ImVec2 displaySize = io.DisplaySize;
    const float longClickThreshold = 0.5f;
    
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if(show_demo_window)
    {
        ImGui::ShowDemoWindow(&show_demo_window);
    }
    
    // Reserved for future use
    //ImGui::Text("%.1f FPS", nvg.io.Framerate);
    

    ImVec2 windowPos = ImVec2((displaySize.x - displaySize.x + 15) * 0.5f,(displaySize.y - displaySize.y + 15) * 0.5f);
    ImGui::SetNextWindowSize(ImVec2(displaySize.x - 15, displaySize.y - 15));    // No size constraints
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::Begin("InvisibleWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    
    ImGui::Columns(3, "ResizableColumns", false); // 2 columns, resizable
    float sb_col_w = ImGui::GetColumnWidth();
    float sb_btn_h = ImGui::GetContentRegionAvail().y;
    if(ImGui::InvisibleButton("<<" , ImVec2(sb_col_w, sb_btn_h)))
        Playback::seek_playback(-Playback::seek_amount);
    
    if(ImGui::IsItemActive())
    {
        float holdDuration = ImGui::GetIO().MouseDownDuration[0]; // [0] is for the left mouse button
    
        // Handling long click / tap event
        if(holdDuration > longClickThreshold)
        {
            main_gui_window = true;
        }
    }
    
    ImGui::NextColumn();

    float ps_col_w = ImGui::GetColumnWidth();
    float ps_btn_h = ImGui::GetContentRegionAvail().y;
    if(ImGui::InvisibleButton(Playback::is_paused ? "|>" : "||" , ImVec2(ps_col_w, ps_btn_h)))
        Playback::pause();
        
    if(ImGui::IsItemActive())
    {
        float holdDuration = ImGui::GetIO().MouseDownDuration[0]; // [0] is for the left mouse button
    
        // Handling long click / tap event to open up the settings window
        if(holdDuration > longClickThreshold)
        {
            main_gui_window = true;
        }
    }    
    
    ImGui::NextColumn();
    
    float sf_col_w = ImGui::GetColumnWidth();
    float sf_btn_h = ImGui::GetContentRegionAvail().y;
    if (ImGui::InvisibleButton(">>" , ImVec2(sf_col_w, sf_btn_h)))
        Playback::seek_playback(Playback::seek_amount);
        
    if(ImGui::IsItemActive())
    {
        float holdDuration = ImGui::GetIO().MouseDownDuration[0]; // [0] is for the left mouse button
    
        // Handling long click / tap event to open up the settings window
        if(holdDuration > longClickThreshold)
        {
            main_gui_window = true;
        }
    }
    
    ImGui::Columns(1); // Reset to single column

    ConstrainWindowMove("PFA Android");
    
    
    
    // Show the main GUI window
    if(main_gui_window)
    {
        ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(center, ImGuiCond_Once, ImVec2(0.5f, 0.5f)); // Pivot 0.5 = center
#ifndef PLATFORM_ANDROID
        ImGui::SetNextWindowSizeConstraints(ImVec2(700, 380), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin("PFA Android", &main_gui_window);
#else   // Setting up a different ui layout for mobile users
        ImGui::SetNextWindowSize(ImVec2(964.0f, 600.0f));
        ImGui::Begin("PFA Android", &main_gui_window, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
#endif

        velocity_filter = live_conf.vel_filter;
        min_velocity = live_conf.vel_min;
        max_velocity = live_conf.vel_max;

        
        if(ImGui::Button("Quit"))
            Exit();
            
        if(ImGui::BeginItemTooltip())
        {
            ImGui::Text("Quit NVi PFA");
            ImGui::EndTooltip();
        }

        if(ImGui::BeginTabBar("tabs", ImGuiTabBarFlags_None))
        {
            if(ImGui::BeginTabItem("Play MIDI Files"))
            {
                ImGui::SetNextItemWidth(310);
                ImGui::InputTextWithHint("##EHE", "Search midis", midi_search, IM_ARRAYSIZE(midi_search));
                
                midi_search_text = midi_search;
                
                ImGui::SameLine();
                
                if(ImGui::Button(ICON_FA_FOLDER_OPEN))
                {
                    IGFD::FileDialogConfig config;
					config.path = last_midi_path;
					config.flags = ImGuiFileDialogFlags_ConfirmOverwrite | ImGuiFileDialogFlags_HideColumnType;
                    ImGuiFileDialog::Instance()->OpenDialog("MidiFileFD", "Choose a MIDI File", ".mid,.midi,.smf,.MID,.MIDI,.SMF", config);
                }
                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Open and play a MIDI file");
                    ImGui::EndTooltip();
                }
                
                ImGui::PushFont(FONT_icon_set);
                ConstrainWindowMove("Choose a MIDI File##MidiFileFD"); // Very smart tweaking XDD
                if(ImGuiFileDialog::Instance()->Display("MidiFileFD", 0, ImVec2(700, 500), ImVec2(FLT_MAX, FLT_MAX)))
                {
                    if(ImGuiFileDialog::Instance()->IsOk())
                    {
                        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                        std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                        last_midi_path = filePath;
                        last_midi_file = filePathName;
                        live_conf.last_midi_path = last_midi_path;
                        live_conf.last_midi_file = last_midi_file;
                        Config::Save(live_conf);
                        Playback::CloseMidi(); // Close previous MIDI file
                        Playback::loadMidiFile(filePathName);
                        
                        
                        if(no_midi_duplicates)
                        {
                            if(!searchDuplicatedMidiItem(live_midi_list, filePathName))
                            {
                                live_midi_list.emplace_back(filePathName);
                                MidiList::save(live_midi_list);
                            }
                            else
                                Log::info("Same midi already exists");
                        }
                        else
                        {
                            live_midi_list.emplace_back(filePathName);
                            MidiList::save(live_midi_list);
                        }
                    }
                    
                    ImGuiFileDialog::Instance()->Close();
                }
                ImGui::PopFont();
                
                ImGui::SameLine();
                
                if(ImGui::Button(ICON_FA_PLAY))
                {
                    Playback::CloseMidi();
                    live_conf.last_midi_file = live_midi_list[selIndex];
                    Config::Save(live_conf);
                    Playback::loadMidiFile(live_midi_list[selIndex]);
                }
                
                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Play the previous MIDI file");
                    ImGui::EndTooltip();
                }
                
                ImGui::SameLine();
                
                if(ImGui::Button(ICON_FA_SQUARE))
                    Playback::CloseMidi();
                
                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Close and reset midi playback");
                    ImGui::EndTooltip();
                }
                
                ImGui::SameLine();
                
                if(ImGui::Button(ICON_FA_CIRCLE_INFO))
                {
                    current_file_info = FileHelpers::GetFileInfo(live_midi_list[selIndex]);
                    file_info_window = true;
                }
                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Show file information");
                    ImGui::EndTooltip();
                }
                
                ImGui::SameLine();
                
                if(ImGui::Button("X"))
                {
                    ImGui::OpenPopup("Clear Midi List Confirmation");
                }
                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Clear midi list");
                    ImGui::EndTooltip();
                }
                
                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                
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
                        MidiList::save(live_midi_list);
                        ImGui::CloseCurrentPopup();
                    }
                    
                    ImGui::EndPopup();
                }
                
                RenderMidiList(live_midi_list, selIndex, midi_search_text);
                ImGui::EndTabItem();
            }
            
            if(ImGui::BeginTabItem("Soundfonts"))
            {
                ImGui::SetNextItemWidth(310);
                ImGui::InputTextWithHint("##XD", "Search soundfonts", sf_search, IM_ARRAYSIZE(sf_search));
                
                sf_search_text = sf_search;
                
                ImGui::SameLine();
                
                if(ImGui::Button("+"))
                {
                    IGFD::FileDialogConfig config;
					config.path = last_sf_path;
                    ImGuiFileDialog::Instance()->OpenDialog("SoundfontFD", "Choose Soundfont File", ".sf2,.sfz,.SF2,.SFZ", config);
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
                        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                        last_sf_path = ImGuiFileDialog::Instance()->GetCurrentPath();
                        live_conf.last_sf_path = last_sf_path;
                        Config::Save(live_conf);
                        
                        if(no_soundfont_duplicates)
                        {
                        
                            if(!searchDuplicatedSoundfontItem(live_soundfont_list, filePathName))
                            {
                                live_soundfont_list.emplace_back(SoundfontItem{filePathName, false});
                                SoundfontList::Save(live_soundfont_list);
                            }
                            else
                                Log::info("Same soundfont already exists");
                        }
                        else
                        {
                            live_soundfont_list.emplace_back(SoundfontItem{filePathName, false});
                            SoundfontList::Save(live_soundfont_list);
                        }
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
                
                ImGui::SameLine();
                
                if(ImGui::Button("-"))
                {
                    live_soundfont_list.erase(live_soundfont_list.begin() + selected_soundfont);
                    SoundfontList::Save(live_soundfont_list);
                }
                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Remove soundfont from the list");
                    ImGui::EndTooltip();
                }
                
                ImGui::SameLine();
                
                if(ImGui::Button(ICON_FA_ARROW_ROTATE_RIGHT))
                {
                    std::vector<std::string> prev_enabled_soundfonts = GetCheckedSoundfonts(live_soundfont_list);
                    live_soundfont_list = SoundfontList::Get(soundfont_paths);
                    
                    // Re-enable previous enabled soundfonts
                    for(size_t i = 0; i < live_soundfont_list.size(); i++)
                    {
                        for(size_t j = 0; j < prev_enabled_soundfonts.size(); j++)
                        {
                            if(prev_enabled_soundfonts[j] == live_soundfont_list[i].label)
                                live_soundfont_list[i].checked = true;
                        }
                    }
                    
                    SoundfontList::Save(live_soundfont_list);
                }
                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Refresh soundfont list");
                    ImGui::EndTooltip();
                }
                
                ImGui::SameLine();
                
                if(ImGui::Button("X"))
                {
                    ImGui::OpenPopup("Confirm clearance");
                }
                if(ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Clear soundfont list");
                    ImGui::EndTooltip();
                }
                
                ImGui::SameLine();
                
                if(ImGui::Button(ICON_FA_CIRCLE_INFO))
                {
                    current_file_info = FileHelpers::GetFileInfo(live_soundfont_list[selected_soundfont].label);
                    file_info_window = true;
                }
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
            
            if(ImGui::BeginTabItem("Settings"))
            {
                if(ImGui::Button("Save"))
                {
                    Config::Save(live_conf);
                }
                if (ImGui::BeginItemTooltip())
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
                        
                        live_conf.no_midi_duplicates = no_midi_duplicates;
                        live_conf.no_soundfont_duplicates = no_soundfont_duplicates;
                        
                        ImGui::Text("Add directories to scan and create soundfont lists");
                        ImGui::InputTextWithHint("##idk", "New entry", soundfons_path_entry, IM_ARRAYSIZE(soundfons_path_entry));
                        ImGui::SameLine();
                        if(ImGui::Button("+##sf"))
                        {
                            if(strlen(soundfons_path_entry) > 0)
                            {
                                if(std::filesystem::exists(soundfons_path_entry))
                                {
                                    soundfont_paths.emplace_back(soundfons_path_entry);
                                    soundfons_path_entry[0] = '\0';
                                }
                                else
                                    ImGui::OpenPopup("Directory Error");
                            }
                        }
                        if(ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Add new soundfonts path entry");
                            ImGui::EndTooltip();
                        }
                        
                        live_conf.extra_sf_paths = soundfont_paths; // Dont forger to update config lol
                        
                        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                        
                        if(ImGui::BeginPopupModal("Directory Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                        {
                            ImGui::Text("Failed to add directory entry to the list !");
                            ImGui::Text("Please make sure the directory path is correct.");
                            
                            float button_width = 120.0f;
                            
                            float window_width = ImGui::GetContentRegionAvail().x;
                            float button_pos_x = (window_width - button_width) * 0.5f;
                            
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + button_pos_x);
                            
                            if(ImGui::Button("OK", ImVec2(button_width, 0)))
                                ImGui::CloseCurrentPopup();
                            
                            ImGui::EndPopup();
                        }
                        
                        ImGui::SameLine();
                            
                        if(ImGui::Button("-##sf1"))
                        {
                            if(selected_soundfont_path_etry >= 0 && selected_soundfont_path_etry < soundfont_paths.size())
                            {
                                soundfont_paths.erase(soundfont_paths.begin() + selected_soundfont_path_etry);
                                selected_soundfont_path_etry = -1;
                            }
                        }
                        if(ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Remove soundfonts path entry");
                            ImGui::EndTooltip();
                        }
                        
                        RenderSoundfontsPathsList(soundfont_paths, selected_soundfont_path_etry);
                        
                        ImGui::EndTabItem();
                    }
                    
                    if(ImGui::BeginTabItem("Visual"))
                    {
                        if(ImGui::Checkbox("Enable vsync *", &vsync))
                            live_conf.vsync = vsync;
                        
                        if(ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Enable or disable vertical synchronization for a smoother frame rate");
                            ImGui::EndTooltip();
                        }
                        
                        ImGui::Checkbox("Overlap Remover *", &overlap_remover);
                        if(ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Removes duplicated note data reducing the visualization lag");
                            ImGui::EndTooltip();
                        }
                        ImGui::SliderInt("Note Speed", &live_note_speed, 100, 20000);
                        ImGui::Text("Background Color");
                        live_conf.OR = overlap_remover;
                        clear_color = ImVec4(live_conf.bg_R / 255.0f, live_conf.bg_G / 255.0f, live_conf.bg_B / 255.0f, live_conf.bg_A / 255.0f);

                        ImGui::ColorEdit3("##H", (float*)&clear_color);

                        if(ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Change the background color of the main scene");
                            ImGui::EndTooltip();
                        }
                        liveColor = UI::Frgba2Irgba(clear_color);
                        
                        ImGui::Checkbox("Vertical Lines", &vertical_lines);
                        live_conf.draw_vertical_lines = vertical_lines;
                        
                        if(ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Show the vertical lines in between the white keys");
                            ImGui::EndTooltip();
                        }
                        
                        ImGui::Text("");
                        ImGui::Text("Custom Channel Colors *");
                        
                        if(ImGui::Button("Reset"))
                        {
                            for(int i = 0; i < 16; i++)
                            {
                                ui_chcolors[i] = UIntToImVec4(NoteColors[i]);
                                live_conf.channel_colors[i] = NoteColors[i];
                            }
                        }
                        if(ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Reset the color order");
                            ImGui::EndTooltip();
                        }
                        
                        for(int i = 0; i < 16; i++)
                        {
                            if(!is_defaultconfig)
                            {
                                ui_chcolors[i] = UIntToImVec4(live_conf.channel_colors[i]);
                            }
                            
                            temp_widget_id = "##Ch" + std::to_string(i);

                            ImGui::ColorEdit3(temp_widget_id.c_str(), (float*)&ui_chcolors[i], ImGuiColorEditFlags_NoInputs);

                            ImGui::SameLine();
                            
                            live_conf.channel_colors[i] = ImVec4ToUInt(ui_chcolors[i]);
                        }
                        ImGui::Text("\n");
                        
                        ImGui::Checkbox("Loop colors *", &loop_colors);
                        live_conf.loop_colors = loop_colors;
                        
                        if(ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("The colors will be applied if track count exceeds 16");
                            ImGui::EndTooltip();
                        }
                        
                        ImGui::Checkbox("Background image", &background_image);
                        live_conf.background_image = background_image;
                        
                        ImGui::SameLine();
                        
                        if(ImGui::Button(ICON_FA_FOLDER_OPEN))
                        {
                            IGFD::FileDialogConfig config;
					        config.path = live_conf.last_image_path;
                            ImGuiFileDialog::Instance()->OpenDialog("ImageFileFD", "Choose Image File", ".png,.jpg,.jpeg,.webp,.bmp,.svg", config);
                        }
                        if(ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Load a background image");
                            ImGui::EndTooltip();
                        }
                        
                        ImGui::Text("Image file: %s", FilenameOnly(live_conf.background_image_path).c_str());
                        
                        ImGui::PushFont(FONT_icon_set);
                        if(ImGuiFileDialog::Instance()->Display("ImageFileFD"))
                        {
                            if(ImGuiFileDialog::Instance()->IsOk())
                            {
                                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                                std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                                
                                live_conf.background_image_path = filePathName;
                                live_conf.last_image_path = filePath;
                                
                                Config::Save(live_conf);
                                
                                RenderWin->LoadBackgroundImage(filePathName);
                            }
                            
                            ImGuiFileDialog::Instance()->Close();
                        }
                        ImGui::PopFont();
                    
                        ImGui::EndTabItem();
                    }
                    
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
                    ImGui::EndTabBar();
                    ImGui::Text("\n");
		        }
		
		        if(ImGui::BeginItemTooltip())
		        {
					ImGui::Text("Set how many notes can be played on specific instruments (changes apply immediately)");
					ImGui::EndTooltip();
		        }
                
                // Keeping the background color updated
                live_conf.bg_R = liveColor.r;
                live_conf.bg_G = liveColor.g;
                live_conf.bg_B = liveColor.b;
                live_conf.bg_A = liveColor.a;
                live_conf.note_speed = live_note_speed;

                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("About"))
            {
                ImGui::BeginChild("ScrollRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
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
    } // Main window
    
    ConstrainWindowMove("File Information");
    
    if(file_info_window)
    {
        ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(center, ImGuiCond_Once, ImVec2(0.5f, 0.5f)); // Pivot 0.5 = center
#ifndef PLATFORM_ANDROID
        ImGui::SetNextWindowSizeConstraints(ImVec2(500, 380), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin("File Information", &file_info_window);
#else   // Setting up a different ui layout for mobile users
        ImGui::SetNextWindowSize(ImVec2(900.0f, 600.0f));
        ImGui::Begin("File Information", &file_info_window, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
#endif

        if(ImGui::BeginTable("File Info table", 3, ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("File");
            ImGui::TableSetupColumn("Size");
            ImGui::TableSetupColumn("Last modification");
            ImGui::TableHeadersRow();
            
            ImGui::TableNextRow();
            
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", current_file_info.file_name.c_str());
            
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", current_file_info.size.c_str());
            
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", current_file_info.last_mod.c_str());
            
            ImGui::EndTable();
        }
        ImGui::End();  
    }
    
    ImGui::End();
    
    // Rendering
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), r);
}