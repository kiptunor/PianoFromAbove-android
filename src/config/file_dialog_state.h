#ifndef FILE_DIALOG_STATE_H
#define FILE_DIALOG_STATE_H

#include <string>

#ifdef PLATFORM_ANDROID
    #define FD_STATE_FILE_PATH "/data/data/com.qsp.nvpfa/files/file_dialog_state.json"
    #define DEFAULT_FD_LOCATION "/storage/emulated/0/Download"
#else
    #define FD_STATE_FILE_PATH "file_dialog_state.json"
    #define DEFAULT_FD_LOCATION "/home"
#endif














class FileDialogState
{
  public:
    static std::string json_file_path;
    typedef struct
    {
        std::string midi_path;
        std::string soundfont_path;
        std::string bg_image_path;
        std::string ccol_path;
        std::string ui_theme_path;
    } DirPaths;

    static void     save(DirPaths dir_paths);
    static DirPaths load();
};

#endif // FILE_DIALOG_STATE_H
