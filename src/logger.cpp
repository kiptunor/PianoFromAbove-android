#include <stdarg.h>
#include <cstdio>
#include <SDL3/SDL_log.h>

#include "logger.h"



void Log::info(const char *src_dbg_str, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, args);
#else
    fprintf(stdout, "%s[\033[38;5;39mINFO\033[0m] -> ", src_dbg_str);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    va_end(args);
}

void Log::warn(const char *src_dbg_str, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, fmt, args);
#else
    fprintf(stdout, "%s[\033[38;5;220mWARN\033[0m] -> ", src_dbg_str);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    va_end(args);
}

void Log::error(const char *src_dbg_str, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, fmt, args);
#else
    fprintf(stdout, "%s[\033[38;5;1mERROR\033[0m] -> ", src_dbg_str);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    va_end(args);
}

void Log::debug(const char *src_dbg_str, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG, fmt, args);
#else
    fprintf(stdout, "%s[\033[38;5;14mDEBUG\033[0m] -> ", src_dbg_str);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    va_end(args);
}

void Log::critical(const char *src_dbg_str, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_CRITICAL, fmt, args);
#else
    fprintf(stdout, "%s[\033[!!!38;5;196mCRITICAL\033[0m!!!] -> ", src_dbg_str);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    va_end(args);
}

void Log::trace(const char *src_dbg_str, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
#ifdef PLATFORM_ANDROID
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_TRACE, fmt, args);
#else
    fprintf(stdout, "%s[\033[38;5;48mTRACE\033[0m] -> ", src_dbg_str);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
#endif
    va_end(args);
}