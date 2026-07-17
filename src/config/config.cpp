#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
// #include <errno>

#include <nlohmann/json.hpp>

#include "../mb_types.h"
#include "config.h"

#include "../globals.h"
#include "../logger.h"














std::string Config::config_path;

// Required for channel / note colors storage
std::string colorToHex(unsigned int color)
{
    int                r = (color >> 16) & 0xFF;
    int                g = (color >> 8) & 0xFF;
    int                b = color & 0xFF;

    std::ostringstream ss;
    ss << "#" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << r << std::setw(2) << g << std::setw(2) << b;

    return ss.str();
}

us_int Config::hexToUInt(const std::string &hex)
{
    std::string clean = hex.substr(1); // Remove '#'
    return static_cast<us_int>(std::strtol(clean.c_str(), nullptr, 16));
}

Config::configuration Config::Load()
{
    std::ifstream         in_file(config_path);
    Config::configuration in_conf;


    if(!in_file)
    {
        Log::error("", "Failed to read configuration '%s', reason: %s", config_path.c_str(), strerror(errno));
        in_conf = default_settings;
        return in_conf;
    }


    in_file.seekg(0, std::ios::end);
    if(in_file.tellg() == 0)
    {
        Log::warn("Config file is empty! Using default settings.");
        return default_settings;
    }
    in_file.seekg(0, std::ios::beg);

    nlohmann::json json_in = nlohmann::json::parse(in_file);

    // clang-format off
    nlohmann::json        general_obj   = json_in.value("general",                   nlohmann::json::object());
    in_conf.use_default_paths           = general_obj.value("useDefaultPaths",       default_settings.use_default_paths);
    in_conf.no_soundfont_duplicates     = general_obj.value("noSoundfontDuplicates", default_settings.no_soundfont_duplicates);
    in_conf.no_midi_duplicates          = general_obj.value("noMidiDuplicates",      default_settings.no_midi_duplicates);
    in_conf.internal_log_buffer         = general_obj.value("internalLogBuffer",     default_settings.internal_log_buffer);
    in_conf.log_to_file                 = general_obj.value("logToFile",             default_settings.log_to_file);

    nlohmann::json prompts_obj          = json_in.value("prompts",            nlohmann::json::object());
    in_conf.dont_show_missing_files     = prompts_obj.value("noMissingFiles", 0);

    nlohmann::json visual_obj           = json_in.value("visual",               nlohmann::json::object());
    in_conf.vsync                       = visual_obj.value("vsync",             default_settings.vsync);
    in_conf.fps                         = visual_obj.value("fps",               default_settings.fps);
    in_conf.note_speed                  = visual_obj.value("noteSpeed",         default_settings.note_speed);
    in_conf.loop_colors                 = visual_obj.value("loopNoteColors",    default_settings.loop_colors);
    in_conf.draw_vertical_lines         = visual_obj.value("drawVerticalLines", default_settings.draw_vertical_lines);
    in_conf.draw_measure_lines          = visual_obj.value("drawMeasureLines",  default_settings.draw_measure_lines);

    in_conf.last_ccol_file_path         = visual_obj.value("channelColors",     "");
    nlohmann::json ui_theme_obj         = visual_obj.value("customUiTheme",     nlohmann::json::object());
    in_conf.custom_ui_theme             = ui_theme_obj.value("enabled",         default_settings.custom_ui_theme);
    in_conf.ui_theme_file_path          = ui_theme_obj.value("path",            default_settings.ui_theme_file_path);

    nlohmann::json builtin_ui_theme_obj = visual_obj.value("builtinUiTheme",    nlohmann::json::object());
    in_conf.builtin_ui_theme            = builtin_ui_theme_obj.value("enabled", default_settings.builtin_ui_theme);
    in_conf.builtin_ui_theme_idx        = builtin_ui_theme_obj.value("index",   default_settings.builtin_ui_theme_idx);

    nlohmann::json background_image_obj = visual_obj.value("backgroundImage",   nlohmann::json::object());
    in_conf.background_image            = background_image_obj.value("enabled", default_settings.background_image);
    in_conf.background_image_path       = background_image_obj.value("path",    default_settings.background_image_path);

    nlohmann::json background_color_obj = visual_obj.value("backgroundColor", nlohmann::json::object());
    in_conf.bg_R                        = background_color_obj.value("R",     default_settings.bg_R);
    in_conf.bg_G                        = background_color_obj.value("G",     default_settings.bg_G);
    in_conf.bg_B                        = background_color_obj.value("B",     default_settings.bg_B);
    in_conf.bg_A                        = background_color_obj.value("A",     default_settings.bg_A);
    in_conf.OR                          = visual_obj.value("overlapRemover",  default_settings.OR);
    in_conf.tick_based_playback         = visual_obj.value("tickBasedPlayback", default_settings.tick_based_playback);

    nlohmann::json audio_obj            = json_in.value("audio",              nlohmann::json::object());
    in_conf.bass_voice_count            = audio_obj.value("voiceCount",       default_settings.bass_voice_count);
    in_conf.audio_device_index          = audio_obj.value("audioDeviceIndex", default_settings.audio_device_index);

    nlohmann::json effects_obj          = audio_obj.value("effects",          nlohmann::json::object());
    nlohmann::json vel_filter_obj       = effects_obj.value("velocityFilter", nlohmann::json::object());
    in_conf.vel_filter                  = vel_filter_obj.value("enabled",     default_settings.vel_filter);
    in_conf.vel_min                     = vel_filter_obj.value("lowVel",      default_settings.vel_min);
    in_conf.vel_max                     = vel_filter_obj.value("hiVel",       default_settings.vel_max);

    nlohmann::json limiter_obj          = effects_obj.value("audioLimiter",   nlohmann::json::object());
    in_conf.audio_limiter               = limiter_obj.value("enabled",        default_settings.audio_limiter);
    // clang-format on

    return in_conf;
}

void Config::Save(configuration config)
{
    nlohmann::ordered_json json_out;

    // clang-format off
    nlohmann::json visual =
    {
        { "vsync",             config.vsync               },
        { "fps",               config.fps                 },
        { "noteSpeed",         config.note_speed          },
        { "loopNoteColors",    config.loop_colors         },
        { "channelColors",     config.last_ccol_file_path },
        { "drawVerticalLines", config.draw_vertical_lines },
        { "drawMeasureLines",  config.draw_measure_lines  },
        { "backgroundImage", {
                { "enabled", config.background_image      },
                { "path",    config.background_image_path }
            }
        },
        { "backgroundColor", {
                { "R", config.bg_R },
                { "G", config.bg_G },
                { "B", config.bg_B },
                { "A", config.bg_A }
            }
        },
        { "overlapRemover",    config.OR                  },
        { "tickBasedPlayback", config.tick_based_playback },
        { "customUiTheme", {
                { "enabled", config.custom_ui_theme },
                { "path", config.ui_theme_file_path }
            }
        },
        { "builtinUiTheme", {
                { "enabled", config.builtin_ui_theme   },
                { "index", config.builtin_ui_theme_idx }
            }
        },
    };

    std::vector<std::string> hex_colors;
    for(i8 i = 0; i < 16; ++i)
        hex_colors.emplace_back(colorToHex(config.channel_colors[i]));


    json_out =
    {
        { "general", {
                { "defaultPaths",          config.use_default_paths       },
                { "noMidiDuplicates",      config.no_midi_duplicates      },
                { "noSoundfontDuplicates", config.no_soundfont_duplicates },
                { "internalLogBuffer",     config.internal_log_buffer     },
                { "logToFile",             config.log_to_file             },
            }
        },
        { "prompts", {
                { "noMissingFiles", config.dont_show_missing_files }
            }
        },
        { "visual",  visual },
        { "audio", {
            { "voiceCount",       config.bass_voice_count   },
            { "audioDeviceIndex", config.audio_device_index },
                { "effects", {
                        { "velocityFilter", {
                                { "enabled", config.vel_filter },
                                { "lowVel",  config.vel_min    },
                                { "hiVel",   config.vel_max    }
                            }
                        },
                        { "audioLimiter", {
                                { "enabled", config.audio_limiter }
                            }
                        }
                    }
                }
            }
        }
    };
    // clang-format on

    std::ofstream          out_file(config_path);
    out_file << json_out.dump(2);

    if(out_file.fail())
        Log::error("", "Failed to save configuration '%s', reason: %s", config_path.c_str(), strerror(errno));
}
