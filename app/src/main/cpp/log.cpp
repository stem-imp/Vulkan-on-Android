//#include "log.hpp"
//#include <cstdarg>
//#include <android/log.h>
//
//using AndroidNative::Log;
//
//string Log::Tag = "android";
//
//void Log::Debug(const char *message, ...)
//{
//    va_list varArgs;
//    va_start(varArgs, message);
//    __android_log_vprint(ANDROID_LOG_DEBUG, Log::Tag.c_str(), message, varArgs);
//    __android_log_print(ANDROID_LOG_DEBUG, Log::Tag.c_str(), "\n");
//    va_end(varArgs);
//}
//
//void Log::Info(const char* message, ...)
//{
//    va_list varArgs;
//    va_start(varArgs, message);
//    __android_log_vprint(ANDROID_LOG_INFO, Log::Tag.c_str(), message, varArgs);
//    __android_log_print(ANDROID_LOG_INFO, Log::Tag.c_str(), "\n");
//    va_end(varArgs);
//}
//
//void Log::Warn(const char *message, ...)
//{
//    va_list varArgs;
//    va_start(varArgs, message);
//    __android_log_vprint(ANDROID_LOG_WARN, Log::Tag.c_str(), message, varArgs);
//    __android_log_print(ANDROID_LOG_WARN, Log::Tag.c_str(), "\n");
//    va_end(varArgs);
//}
//
//void Log::Error(const char *message, ...)
//{
//    va_list varArgs;
//    va_start(varArgs, message);
//    __android_log_vprint(ANDROID_LOG_ERROR, Log::Tag.c_str(), message, varArgs);
//    __android_log_print(ANDROID_LOG_ERROR, Log::Tag.c_str(), "\n");
//    va_end(varArgs);
//}