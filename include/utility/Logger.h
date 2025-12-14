#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog/logger.h>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

namespace Internal
{

/**
 * @brief Centralized logging system for the game engine
 *
 * @description
 * Features:
 * - File-based logging
 * - Timestamped log files (e.g., 2025-12-13_14-30-45.log)
 * - Compile-time log level stripping for Release builds
 * - Optional console output for game developers debugging their games
 * - Structured logging for entities and components
 */
class Logger
{
public:
    /**
     * @brief Initialize the logging system
     * @param logDirectory Directory to store log files relative to executable (created if doesn't exist)
     */
    static void initialize(const std::string& logDirectory = "logs");

    /**
     * @brief Shutdown the logging system and flush all pending messages
     */
    static void shutdown();

    /**
     * @brief Check if logger is initialized
     */
    static bool isInitialized();

    /**
     * @brief Log an info-level message (file only)
     */
    template <typename... Args>
    static void info(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if (s_logger)
        {
            s_logger->info(fmt, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log an info-level message to both file and console
     */
    template <typename... Args>
    static void infoConsole(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if (s_logger)
        {
            s_logger->info(fmt, std::forward<Args>(args)...);
        }
        if (s_consoleLogger)
        {
            s_consoleLogger->info(fmt, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log an error-level message (file only)
     */
    template <typename... Args>
    static void error(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if (s_logger)
        {
            s_logger->error(fmt, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log an error-level message to both file and console
     */
    template <typename... Args>
    static void errorConsole(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if (s_logger)
        {
            s_logger->error(fmt, std::forward<Args>(args)...);
        }
        if (s_consoleLogger)
        {
            s_consoleLogger->error(fmt, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log a warning-level message (file only)
     */
    template <typename... Args>
    static void warn(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if (s_logger)
        {
            s_logger->warn(fmt, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log a warning-level message to both file and console
     */
    template <typename... Args>
    static void warnConsole(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if (s_logger)
        {
            s_logger->warn(fmt, std::forward<Args>(args)...);
        }
        if (s_consoleLogger)
        {
            s_consoleLogger->warn(fmt, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log a debug-level message (file only)
     */
    template <typename... Args>
    static void debug(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if (s_logger)
        {
            s_logger->debug(fmt, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Log a debug-level message to both file and console
     */
    template <typename... Args>
    static void debugConsole(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if (s_logger)
        {
            s_logger->debug(fmt, std::forward<Args>(args)...);
        }
        if (s_consoleLogger)
        {
            s_consoleLogger->debug(fmt, std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Flush all pending log messages to file
     */
    static void flush();

    /**
     * @brief Get the path to the current log file
     */
    static std::string getCurrentLogPath();

private:
    static std::shared_ptr<spdlog::logger> s_logger;         ///< File logger
    static std::shared_ptr<spdlog::logger> s_consoleLogger;  ///< Console logger
    static std::string                     s_currentLogPath;

    /**
     * @brief Generate timestamped filename
     */
    static std::string generateTimestampedFilename();

    Logger() = delete;
};

}  // namespace Internal

// Expose Logger at global scope for convenience
using Logger = Internal::Logger;

// ============================================================================
// Logging Macros with Compile-Time Elimination
// ============================================================================

// Logging macros.
// - INFO/WARN/ERROR are enabled in all builds.
// - DEBUG is compiled out in Release (NDEBUG).

#define LOG_INFO(...) Internal::Logger::info(__VA_ARGS__)
#define LOG_WARN(...) Internal::Logger::warn(__VA_ARGS__)
#define LOG_ERROR(...) Internal::Logger::error(__VA_ARGS__)

#ifndef NDEBUG
#define LOG_DEBUG(...) Internal::Logger::debug(__VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

// Convenience macros for console output.
// Usage: LOG_INFO_CONSOLE("message {}", value);

#define LOG_INFO_CONSOLE(...) Internal::Logger::infoConsole(__VA_ARGS__)
#define LOG_WARN_CONSOLE(...) Internal::Logger::warnConsole(__VA_ARGS__)
#define LOG_ERROR_CONSOLE(...) Internal::Logger::errorConsole(__VA_ARGS__)

#ifndef NDEBUG
#define LOG_DEBUG_CONSOLE(...) Internal::Logger::debugConsole(__VA_ARGS__)
#else
#define LOG_DEBUG_CONSOLE(...) ((void)0)
#endif

#endif  // LOGGER_H
