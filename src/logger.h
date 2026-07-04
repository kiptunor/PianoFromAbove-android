#ifndef LOGGER_H
#define LOGGER_H


#include <string>
#include <vector>


#ifdef PLATFORM_ANDROID
    #define LOG_FILE_DIR "/data/data/com.qsp.nvpfa/files/logs/"
#else
    #define LOG_FILE_DIR "/.cache/npfa/logs"
#endif


#define MAX_LOG_BUFFER 5024

#ifndef PLATFORM_ANDROID
    #define SRC_STRING ((std::string(__FILE__) + ": " + std::to_string(__LINE__) + ": " + __func__ + " | ")).c_str()
#endif














class Log
{
  public:
    // static bool log_to_file;
    static bool                     log_to_stdout;
    static bool                     log_to_internal_buf;

    static void                     createFile(const char *filename);
    static void                     closeFile();
    static void                     info(const char *src_dbg_str, const char *fmt, ...);
    static void                     warn(const char *src_dbg_str, const char *fmt, ...);
    static void                     error(const char *src_dbg_str, const char *fmt, ...);
    static void                     critical(const char *src_dbg_str, const char *fmt, ...);
    static void                     debug(const char *src_dbg_str, const char *fmt, ...);
    static void                     trace(const char *src_dbg_str, const char *fmt, ...);

    // No source logging
    static void                     info(const char *fmt, ...);
    static void                     warn(const char *fmt, ...);
    static void                     error(const char *fmt, ...);
    static void                     critical(const char *fmt, ...);
    static void                     debug(const char *fmt, ...);
    static void                     trace(const char *fmt, ...);

    static std::vector<std::string> log_buffer;
    static std::string              last_log;
};


#endif
