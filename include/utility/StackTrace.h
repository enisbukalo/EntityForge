#ifndef STACKTRACE_H
#define STACKTRACE_H

#include <string>

namespace Internal
{

/**
 * @brief Captures and formats stack traces for error logging
 *
 * @description
 * Platform-specific implementation:
 * - Windows: Uses CaptureStackBackTrace + SymFromAddr
 * - Linux: Uses backtrace + backtrace_symbols
 *
 * Stack traces are only captured on errors to minimize performance impact.
 */
class StackTrace
{
public:
    /**
     * @brief Capture current stack trace
     * @param skipFrames Number of frames to skip from the top (default 1 to skip this call)
     * @param maxFrames Maximum number of frames to capture
     * @return Formatted stack trace string
     */
    static std::string capture(int skipFrames = 1, int maxFrames = 16);

private:
    StackTrace() = delete;
};

}  // namespace Internal

#endif  // STACKTRACE_H
