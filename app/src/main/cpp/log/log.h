#ifndef UTILITY_LOG_H
#define UTILITY_LOG_H

#include <string>

using std::string;

namespace Utility {
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

#endif // UTILITY_LOG_H
