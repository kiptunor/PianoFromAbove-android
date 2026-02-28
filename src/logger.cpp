#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

#include <stdarg.h>
#include <cstdio>
#include <SDL3/SDL_log.h>

#include "logger.h"





bool Log::log_to_stdout = true;
bool Log::log_to_internal_buf = true;
std::vector<std::string> Log::log_buffer;
std::string Log::last_log;

std::ofstream log_file;
bool is_log_file_opened = false;


void push_log_buffer(const std::string str)
{
    Log::last_log = str;
    Log::log_buffer.emplace_back(str);
}

void Log::createFile(const char *filename)
{
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << filename << std::put_time(std::localtime(&time), "_%Y-%m-%d_%H:%M:%S.log");
    std::string str = ss.str();
    
    log_file.open(ss.str());
    is_log_file_opened = true;
}

void Log::closeFile()
{
    if(is_log_file_opened)
        log_file.close();
}

/*
Ugly code but it does its job :/
*/

void Log::info(const char *src_dbg_str, const char *fmt, ...)
{
    va_list args, args_copy;
    char buf[MAX_LOG_BUFFER];
    va_start(args, fmt);
    va_copy(args_copy, args);
    vsnprintf(buf, sizeof(buf), fmt, args_copy);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, args);
#else
    fprintf(stdout, "%s[\033[38;5;39mINFO\033[0m] -> ", src_dbg_str);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    
    if(is_log_file_opened)
        log_file << src_dbg_str << "[INFO] -> " << buf << "\n";
    
    if(log_to_internal_buf)
        push_log_buffer(src_dbg_str + std::string("[INFO] -> ") + std::string(buf));
    
    va_end(args);
    va_end(args_copy);
}

void Log::warn(const char *src_dbg_str, const char *fmt, ...)
{
    va_list args, args_copy;
    char buf[MAX_LOG_BUFFER];
    va_start(args, fmt);
    va_copy(args_copy, args);
    vsnprintf(buf, sizeof(buf), fmt, args_copy);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, fmt, args);
#else
    fprintf(stdout, "%s[\033[38;5;220mWARN\033[0m] -> ", src_dbg_str);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    
    if(is_log_file_opened)
        log_file << src_dbg_str << "[WARN] -> " << buf << "\n";
    
    if(log_to_internal_buf)
        push_log_buffer(src_dbg_str + std::string("[WARN] -> ") + std::string(buf));
    
    va_end(args);
    va_end(args_copy);
}

void Log::error(const char *src_dbg_str, const char *fmt, ...)
{
    va_list args, args_copy;
    char buf[MAX_LOG_BUFFER];
    va_start(args, fmt);
    va_copy(args_copy, args);
    vsnprintf(buf, sizeof(buf), fmt, args_copy);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, fmt, args);
#else
    fprintf(stdout, "%s[\033[38;5;1mERROR\033[0m] -> ", src_dbg_str);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    
    if(is_log_file_opened)
        log_file << src_dbg_str << "[ERROR] -> " << buf << "\n";
    
    if(log_to_internal_buf)
        push_log_buffer(src_dbg_str + std::string("[ERROR] -> ") + std::string(buf));
    
    va_end(args);
    va_end(args_copy);
}

void Log::debug(const char *src_dbg_str, const char *fmt, ...)
{
    va_list args, args_copy;
    char buf[MAX_LOG_BUFFER];
    va_start(args, fmt);
    va_copy(args_copy, args);
    vsnprintf(buf, sizeof(buf), fmt, args_copy);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG, fmt, args);
#else
    fprintf(stdout, "%s[\033[38;5;14mDEBUG\033[0m] -> ", src_dbg_str);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    
    if(is_log_file_opened)
        log_file << src_dbg_str << "[DEBUG] -> " << buf << "\n";
    
    if(log_to_internal_buf)
        push_log_buffer(src_dbg_str + std::string("[DEBUG] -> ") + std::string(buf));
    
    va_end(args);
    va_end(args_copy);
}

void Log::critical(const char *src_dbg_str, const char *fmt, ...)
{
    va_list args, args_copy;
    char buf[MAX_LOG_BUFFER];
    va_start(args, fmt);
    va_copy(args_copy, args);
    vsnprintf(buf, sizeof(buf), fmt, args_copy);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_CRITICAL, fmt, args);
#else
    fprintf(stdout, "%s[!!!\033[38;5;196mCRITICAL\033[0m!!!] -> ", src_dbg_str);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    
    if(is_log_file_opened)
        log_file << src_dbg_str << "[!!!CRITICAL!!!] -> " << buf << "\n";
    
    if(log_to_internal_buf)
        push_log_buffer(src_dbg_str + std::string("[!!!CRITICAL!!!] -> ") + std::string(buf));
    
    va_end(args);
    va_end(args_copy);
}

void Log::trace(const char *src_dbg_str, const char *fmt, ...)
{
    va_list args, args_copy;
    char buf[MAX_LOG_BUFFER];
    va_start(args, fmt);
    va_copy(args_copy, args);
    vsnprintf(buf, sizeof(buf), fmt, args_copy);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_TRACE, fmt, args);
#else
    fprintf(stdout, "%s[\033[38;5;48mTRACE\033[0m] -> ", src_dbg_str);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    
    if(is_log_file_opened)
        log_file << src_dbg_str << "[TRACE] -> " << buf << "\n";
    
    if(log_to_internal_buf)
        push_log_buffer(src_dbg_str + std::string("[TRACE] -> ") + std::string(buf));
    
    va_end(args);
    va_end(args_copy);
}


/*
    ▗▖  ▗▖ ▗▄▖      ▗▄▄▖ ▗▄▖ ▗▖ ▗▖▗▄▄▖  ▗▄▄▖▗▄▄▄▖
    ▐▛▚▖▐▌▐▌ ▐▌    ▐▌   ▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌▐▌   ▐▌   
    ▐▌ ▝▜▌▐▌ ▐▌     ▝▀▚▖▐▌ ▐▌▐▌ ▐▌▐▛▀▚▖▐▌   ▐▛▀▀▘
    ▐▌  ▐▌▝▚▄▞▘    ▗▄▄▞▘▝▚▄▞▘▝▚▄▞▘▐▌ ▐▌▝▚▄▄▖▐▙▄▄▖
                                                 
                                                                                 
    ▗▖    ▗▄▖  ▗▄▄▖ ▗▄▄▖▗▄▄▄▖▗▖  ▗▖ ▗▄▄▖         
    ▐▌   ▐▌ ▐▌▐▌   ▐▌     █  ▐▛▚▖▐▌▐▌            
    ▐▌   ▐▌ ▐▌▐▌▝▜▌▐▌▝▜▌  █  ▐▌ ▝▜▌▐▌▝▜▌         
    ▐▙▄▄▖▝▚▄▞▘▝▚▄▞▘▝▚▄▞▘▗▄█▄▖▐▌  ▐▌▝▚▄▞▘         
                                                 
                                                 
                                                 
    Here the loging functions don't require first argument where you specify the macro that concatenates source debugging (See logger.h:15)
*/


void Log::info(const char *fmt, ...)
{
    va_list args, args_copy;
    char buf[MAX_LOG_BUFFER];
    va_start(args, fmt);
    va_copy(args_copy, args);
    vsnprintf(buf, sizeof(buf), fmt, args_copy);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, args);
#else
    fprintf(stdout, "[\033[38;5;39mINFO\033[0m] -> ");
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    
    if(is_log_file_opened)
        log_file << "[INFO] -> " << buf << "\n";
    
    if(log_to_internal_buf)
        push_log_buffer(std::string("[INFO] -> ") + std::string(buf));
    
    va_end(args);
    va_end(args_copy);
}

void Log::warn(const char *fmt, ...)
{
    va_list args, args_copy;
    char buf[MAX_LOG_BUFFER];
    va_start(args, fmt);
    va_copy(args_copy, args);
    vsnprintf(buf, sizeof(buf), fmt, args_copy);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, fmt, args);
#else
    fprintf(stdout, "[\033[38;5;220mWARN\033[0m] -> ");
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    
    if(is_log_file_opened)
        log_file << "[WARN] -> " << buf << "\n";
    
    if(log_to_internal_buf)
        push_log_buffer(std::string("[WARN] -> ") + std::string(buf));
    
    va_end(args);
    va_end(args_copy);
}

void Log::error(const char *fmt, ...)
{
    va_list args, args_copy;
    char buf[MAX_LOG_BUFFER];
    va_start(args, fmt);
    va_copy(args_copy, args);
    vsnprintf(buf, sizeof(buf), fmt, args_copy);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, fmt, args);
#else
    fprintf(stdout, "[\033[38;5;1mERROR\033[0m] -> ");
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    
    if(is_log_file_opened)
        log_file << "[ERROR] -> " << buf << "\n";
    
    if(log_to_internal_buf)
        push_log_buffer(std::string("[ERROR] -> ") + std::string(buf));

    va_end(args);
    va_end(args_copy);
}

void Log::debug(const char *fmt, ...)
{
    va_list args, args_copy;
    char buf[MAX_LOG_BUFFER];
    va_start(args, fmt);
    va_copy(args_copy, args);
    vsnprintf(buf, sizeof(buf), fmt, args_copy);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG, fmt, args);
#else
    fprintf(stdout, "[\033[38;5;14mDEBUG\033[0m] -> ");
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    
    if(is_log_file_opened)
        log_file << "[DEBUG] -> " << buf << "\n";
    
    if(log_to_internal_buf)
        push_log_buffer(std::string("[DEBUG] -> ") + std::string(buf));
    
    va_end(args);
    va_end(args_copy);
}

void Log::critical(const char *fmt, ...)
{
    va_list args, args_copy;
    char buf[MAX_LOG_BUFFER];
    va_start(args, fmt);
    va_copy(args_copy, args);
    vsnprintf(buf, sizeof(buf), fmt, args_copy);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_CRITICAL, fmt, args);
#else
    fprintf(stdout, "[!!!\033[38;5;196mCRITICAL\033[0m!!!] -> ");
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    
    if(is_log_file_opened)
        log_file << "[!!!CRITICAL!!!] -> " << buf << "\n";
    
    if(log_to_internal_buf)
        push_log_buffer(std::string("[!!!CRITICAL!!!] -> ") + std::string(buf));
    
    va_end(args);
    va_end(args_copy);
}

void Log::trace(const char *fmt, ...)
{
    va_list args, args_copy;
    char buf[MAX_LOG_BUFFER];
    va_start(args, fmt);
    va_copy(args_copy, args);
    vsnprintf(buf, sizeof(buf), fmt, args_copy);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_TRACE, fmt, args);
#else
    fprintf(stdout, "[\033[38;5;48mTRACE\033[0m] -> ");
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    
    if(is_log_file_opened)
        log_file << "[TRACE] -> " << buf << "\n";
    
    if(log_to_internal_buf)
        push_log_buffer(std::string("[TRACE] -> ") + std::string(buf));
    
    va_end(args);
    va_end(args_copy);
}