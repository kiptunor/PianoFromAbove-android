#include <nlohmann/json.hpp>
#include <fstream>


#include "file_dialog_state.h"
#include "../file_helpers.h"




std::string FileDialogState::json_file_path;



FileDialogState::DirPaths FileDialogState::load()
{
    FileDialogState::DirPaths dir_paths;
    std::ifstream in_file(json_file_path);
        
    nlohmann::json json_in = nlohmann::json::parse(in_file);
    
    nlohmann::json dir_paths_obj = json_in.value("directoryPaths", nlohmann::json::object());
    
    dir_paths.midi_path      = dir_paths_obj.value("midi",      "");
    dir_paths.soundfont_path = dir_paths_obj.value("soundfont", "");
    dir_paths.bg_image_path     = dir_paths_obj.value("bgImage",   "");
    dir_paths.ccol_path      = dir_paths_obj.value("ccol",      "");
    dir_paths.ui_theme_path  = dir_paths_obj.value("uiTheme",   "");
    
    return dir_paths;
}

void FileDialogState::save(DirPaths dir_paths)
{
    nlohmann::ordered_json json_out;
    
    json_out = {
        { "directoryPaths", {
            { "midi",      dir_paths.midi_path      },
            { "soundfont", dir_paths.soundfont_path },
            { "bgImage",   dir_paths.bg_image_path  },
            { "ccol",      dir_paths.ccol_path      },
            { "uiTheme",   dir_paths.ui_theme_path  }
        }},
    };
    
    std::ostringstream f_path;
    
#ifndef PLATFORKM_ANDROID
    f_path << FileHelpers::lists_dir << "/" << FD_STATE_FILE_PATH;
#else
    f_path << FD_STATE_FILE_PATH;
#endif

    std::ofstream out_file(f_path.str());
    out_file << json_out;
}