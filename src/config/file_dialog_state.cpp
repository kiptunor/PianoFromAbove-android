#include <fstream>
#include <string>
#include <nlohmann/json.hpp>


#include "../file_helpers.h"
#include "../logger.h"
#include "file_dialog_state.h"














std::string               FileDialogState::json_file_path;







FileDialogState::DirPaths FileDialogState::load()
{
    Log::debug("Loading FD State...");
    FileDialogState::DirPaths dir_paths;
    
    
    

    
    
    
    
    std::ifstream             in_file(json_file_path);

    in_file.seekg(0, std::ios::end);
    if(in_file.tellg() == 0 || !in_file.good())
    {
        Log::warn("File dialog state is empty or not found!");
        dir_paths.midi_path          = DEFAULT_FD_LOCATION;
        dir_paths.soundfont_path     = DEFAULT_FD_LOCATION;
        dir_paths.bg_image_path      = DEFAULT_FD_LOCATION;
        dir_paths.ccol_path          = DEFAULT_FD_LOCATION;
        dir_paths.ui_theme_path      = DEFAULT_FD_LOCATION;
        return dir_paths;
    }
    in_file.seekg(0, std::ios::beg);

    nlohmann::json json_in       = nlohmann::json::parse(in_file);

    nlohmann::json dir_paths_obj = json_in.value("directoryPaths", nlohmann::json::object());

    dir_paths.midi_path          = dir_paths_obj.value("midi",      DEFAULT_FD_LOCATION);
    dir_paths.soundfont_path     = dir_paths_obj.value("soundfont", DEFAULT_FD_LOCATION);
    dir_paths.bg_image_path      = dir_paths_obj.value("bgImage",   DEFAULT_FD_LOCATION);
    dir_paths.ccol_path          = dir_paths_obj.value("ccol",      DEFAULT_FD_LOCATION);
    dir_paths.ui_theme_path      = dir_paths_obj.value("uiTheme",   DEFAULT_FD_LOCATION);

    return dir_paths;
}

void FileDialogState::save(DirPaths dir_paths)
{
    nlohmann::ordered_json json_out;

    // clang-format off
    json_out = {
        { "directoryPaths",
            {
                { "midi",      dir_paths.midi_path      },
                { "soundfont", dir_paths.soundfont_path },
                { "bgImage",   dir_paths.bg_image_path  },
                { "ccol",      dir_paths.ccol_path      },
                { "uiTheme",   dir_paths.ui_theme_path  }
            }
        },
    };
    // clang-format on

    std::ostringstream     f_path;

#ifndef PLATFORM_ANDROID
    f_path << FileHelpers::lists_dir << "/" << FD_STATE_FILE_PATH;
#else
    f_path << FD_STATE_FILE_PATH;
#endif

    std::ofstream out_file(f_path.str());
    out_file << json_out;
    
    if(out_file.fail())
        Log::error("", "Failed to save file dialog state '%s', reason: %s", f_path.str().c_str(), strerror(errno));
}
