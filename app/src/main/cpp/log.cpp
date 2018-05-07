#include "log.hpp"
#include <cstdarg>
#include <android/log.h>

string Log::TAG = "android";

void Log::Debug(const char *message, ...)
{
    va_list varArgs;
    va_start(varArgs, message);
    __android_log_vprint(ANDROID_LOG_DEBUG, Log::TAG.c_str(), message, varArgs);
    __android_log_print(ANDROID_LOG_DEBUG, Log::TAG.c_str(), "\n");
    va_end(varArgs);
}

void Log::Info(const char* message, ...)
{
    va_list varArgs;
    va_start(varArgs, message);
    __android_log_vprint(ANDROID_LOG_INFO, Log::TAG.c_str(), message, varArgs);
    __android_log_print(ANDROID_LOG_INFO, Log::TAG.c_str(), "\n");
    va_end(varArgs);
}

void Log::Warn(const char *message, ...)
{
    va_list varArgs;
    va_start(varArgs, message);
    __android_log_vprint(ANDROID_LOG_WARN, Log::TAG.c_str(), message, varArgs);
    __android_log_print(ANDROID_LOG_WARN, Log::TAG.c_str(), "\n");
    va_end(varArgs);
}

void Log::Error(const char *message, ...)
{
    va_list varArgs;
    va_start(varArgs, message);
    __android_log_vprint(ANDROID_LOG_ERROR, Log::TAG.c_str(), message, varArgs);
    __android_log_print(ANDROID_LOG_ERROR, Log::TAG.c_str(), "\n");
    va_end(varArgs);
}