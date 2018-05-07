#ifndef ANDROID_LOG_HPP
#define ANDROID_LOG_HPP

#include <string>

using std::string;

class Log
{
public:
    static string TAG;

    static void Error(const char* message, ...);
    static void Warn(const char* message, ...);
    static void Info(const char* message, ...);
    static void Debug(const char* message, ...);
};

#ifndef NDEBUG
#define app_log(...) Log::Debug(__VA_ARGS__)
#else
#define app_log(...)
#endif

#endif // ANDROID_LOG_HPP
