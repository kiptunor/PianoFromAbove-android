#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>

#include <nlohmann/json.hpp>


#include "config.h"




// Required for channel / note colors storage
std::string colorToHex(unsigned int color)
{
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;
        
    std::ostringstream ss;
    ss << "#" << std::uppercase << std::hex << std::setfill('0') 
    << std::setw(2) << r 
    << std::setw(2) << g 
    << std::setw(2) << b;
    
    return ss.str();
}

unsigned int hexToUInt(const std::string& hex)
{
    std::string clean = hex.substr(1);  // Remove '#'
    return static_cast<unsigned int>(std::strtol(clean.c_str(), nullptr, 16));
}



Config::configuration Config::Load()
{
    std::ifstream in_file(CONFIG_FILE_PATH);
        
    nlohmann::json json_in = nlohmann::json::parse(in_file);
        
    Config::configuration in_conf;
    
    nlohmann::json general_obj = json_in.value("general", nlohmann::json::object());
    in_conf.use_default_paths = general_obj.value("useDefaultPaths", default_settings.use_default_paths);
    in_conf.last_midi_path = general_obj.value("lastMidiPath", "");
    in_conf.last_midi_file = general_obj.value("lastMidiFilePath", "");
    
    nlohmann::json visual_obj = json_in.value("visual", nlohmann::json::object());
    in_conf.vsync = visual_obj.value("vsync", default_settings.vsync);
    in_conf.note_speed = visual_obj.value("noteSpeed", default_settings.note_speed);
    in_conf.loop_colors = visual_obj.value("loopNoteColors", default_settings.loop_colors);
    int count = std::min(visual_obj["channelColors"].size(), (size_t)16);
    if(count != 0)
        in_conf.is_custom_ch_colors = true;
    
    for(int i = 0; i < count; ++i)
    {
        std::string hexStr = visual_obj["channelColors"][i].get<std::string>();
        in_conf.channel_colors[i] = hexToUInt(hexStr);
    }
    
    nlohmann::json background_image_obj = visual_obj.value("backgroundImage", nlohmann::json::object());
    in_conf.use_bg_img = background_image_obj.value("enabled", default_settings.use_bg_img);
    in_conf.bg_img = background_image_obj.value("path", default_settings.bg_img);
    
    nlohmann::json background_color_obj = visual_obj.value("backgroundColor", nlohmann::json::object());
    in_conf.bg_R = background_color_obj.value("R", default_settings.bg_R);
    in_conf.bg_G = background_color_obj.value("G", default_settings.bg_G);
    in_conf.bg_B = background_color_obj.value("B", default_settings.bg_B);
    in_conf.bg_A = background_color_obj.value("A", default_settings.bg_A);
    in_conf.OR = visual_obj.value("overlapRemover", default_settings.OR);
    
    nlohmann::json audio_obj = json_in.value("audio", nlohmann::json::object());
    in_conf.bass_voice_count = audio_obj.value("voiceCount", default_settings.bass_voice_count);
    in_conf.audio_device_index = audio_obj.value("audioDeviceIndex", default_settings.audio_device_index);
    
    nlohmann::json effects_obj = audio_obj.value("effects", nlohmann::json::object());
    nlohmann::json vel_filter_obj = effects_obj.value("velocityFilter", nlohmann::json::object());
    in_conf.vel_filter = vel_filter_obj.value("enabled", default_settings.vel_filter);
    in_conf.vel_min = vel_filter_obj.value("lowVel", default_settings.vel_min);
    in_conf.vel_max = vel_filter_obj.value("hiVel", default_settings.vel_max);
    
    nlohmann::json limiter_obj = effects_obj.value("audioLimiter", nlohmann::json::object());
    in_conf.audio_limiter = limiter_obj.value("enabled", default_settings.audio_limiter);
    
    
    return in_conf;
}


void Config::Save(configuration config)
{
    nlohmann::ordered_json json_out;
        
    nlohmann::json visual = {
        { "vsync", config.vsync },
        { "noteSpeed", config.note_speed },
        { "loopNoteColors", config.loop_colors },
        { "backgroundImage", {
            { "enabled", config.use_bg_img },
            { "path", config.bg_img }
        }},
        { "backgroundColor", {
            { "R", config.bg_R },
            { "G", config.bg_G },
            { "B", config.bg_B },
            { "A", config.bg_A }
        }},
        { "overlapRemover", config.OR }
    };
        
    std::vector<std::string> colorHexes;
    for(int i = 0; i < 16; ++i)
        colorHexes.push_back(colorToHex(config.channel_colors[i]));
    
    visual["channelColors"] = colorHexes;
        
    json_out = {
        { "general", {
            { "defaultPaths", config.use_default_paths },
            { "lastMidiPath", config.last_midi_path },
            { "lastMidiFilePath", config.last_midi_file }
        }},
        { "visual", visual },
        { "audio", {
            { "voiceCount", config.bass_voice_count },
            { "audioDeviceIndex", config.audio_device_index },
            { "effects", {
                { "velocityFilter", {
                    { "enabled", config.vel_filter },
                    { "lowVel", config.vel_min },
                    { "hiVel", config.vel_max }
                }},
                { "audioLimiter", {
                    { "enabled", config.audio_limiter }
                }}
            }}
        }}
    };
        
    std::ofstream out_file(CONFIG_FILE_PATH);
    out_file << json_out.dump(2);
}

//#ifdef DEBUG
void Config::PrintLoadedConfig(configuration c)
{
    std::cout << "Loaded Configuration:" << std::endl;
    std::cout << "Default Paths: " << c.use_default_paths << std::endl;
    std::cout << "Background Image Enabled: " << c.use_bg_img << std::endl;
    std::cout << "Background Image Path: " << c.bg_img << std::endl;
    std::cout << "Background Color: (" << c.bg_R << ", " << c.bg_G << ", " << c.bg_B << ", " << c.bg_A << ")" << std::endl;
    std::cout << "Note Speed: " << c.note_speed << std::endl;
    std::cout << "Loop Note Colors: " << c.loop_colors << std::endl;
    std::cout << "Overlap Remover: " << c.OR << std::endl;
    std::cout << "Channel Colors: ";
    for(int i = 0; i < 16; ++i)
        std::cout << colorToHex(c.channel_colors[i]) << " ";
    
    std::cout << std::endl;
    std::cout << "Bass Voice Count: " << c.bass_voice_count << std::endl;
    std::cout << "Audio Device Index: " << c.audio_device_index << std::endl;
    std::cout << "Velocity Filter Enabled: " << c.vel_filter << std::endl;
    std::cout << "Velocity Filter Min: " << c.vel_min << std::endl;
    std::cout << "Velocity Filter Max: " << c.vel_max << std::endl;
    std::cout << "Audio Limiter Enabled: " << c.audio_limiter << std::endl;
}