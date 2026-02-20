#ifndef LOGGER_H
#define LOGGER_H




#ifndef PLATFORM_ANDROID
    #define SRC_STRING \
    ((std::string(__FILE__) + ": " + \
          std::to_string(__LINE__) + ": " + \
          __func__ + " | "))
#endif

class Log
{
    public:
        static void createFile();
        static void info(const char *src_dbg_str = "", const char *fmt = "", ...);
        static void warn(const char *src_dbg_str = "", const char *fmt = "", ...);
        static void error(const char *src_dbg_str = "", const char *fmt = "", ...);
        static void critical(const char *src_dbg_str = "", const char *fmt = "", ...);
        static void debug(const char *src_dbg_str = "", const char *fmt = "", ...);
        static void trace(const char *src_dbg_str = "", const char *fmt = "", ...);
};


#endif