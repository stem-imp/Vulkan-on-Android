#ifndef ANDROID_LOG_HPP
#define ANDROID_LOG_HPP

#include <string>
#include <stdexcept>

using std::string;
using std::to_string;
using std::runtime_error;

namespace AndroidNative {
    class Log
    {
    public:
        static string Tag;

        static void Error(const char *message, ...);
        static void Warn(const char *message, ...);
        static void Info(const char *message, ...);
        static void Debug(const char *message, ...);
    };
}

#ifndef NDEBUG
#define DebugLog(...) Log::Debug(__VA_ARGS__)
#else
#define DebugLog(...)
#endif

#define InfoLog(...) Log::Info(__VA_ARGS__)

#endif // ANDROID_LOG_HPP
