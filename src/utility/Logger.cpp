#include "Logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>

#if defined(_WIN32)
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace Internal
{

std::shared_ptr<spdlog::logger> Logger::s_logger         = nullptr;
std::shared_ptr<spdlog::logger> Logger::s_consoleLogger  = nullptr;
std::string                     Logger::s_currentLogPath = "";

namespace
{

std::filesystem::path getExecutableDir()
{
#if defined(_WIN32)
    char        buffer[MAX_PATH] = {0};
    const DWORD len              = GetModuleFileNameA(nullptr, buffer, static_cast<DWORD>(sizeof(buffer)));
    if (len == 0 || len >= sizeof(buffer))
    {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(std::string(buffer, len)).parent_path();
#else
    char          buffer[PATH_MAX] = {0};
    const ssize_t len              = ::readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len <= 0)
    {
        return std::filesystem::current_path();
    }
    buffer[len] = '\0';
    return std::filesystem::path(buffer).parent_path();
#endif
}

std::filesystem::path resolveLogDir(const std::string& logDirectory)
{
    std::filesystem::path dir(logDirectory);
    if (dir.is_relative())
    {
        dir = getExecutableDir() / dir;
    }
    return dir;
}

}  // namespace

std::string Logger::generateTimestampedFilename()
{
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d_%H-%M-%S");
    return ss.str() + ".log";
}

void Logger::initialize(const std::string& logDirectory)
{
    if (s_logger)
    {
        return;  // Already initialized
    }

    try
    {
        // Always create console logger first, so we can report failures even if file logging can't be set up.
        try
        {
            if (auto existing = spdlog::get("ConsoleLogger"))
            {
                s_consoleLogger = existing;
            }
            else
            {
                s_consoleLogger = spdlog::stdout_color_mt("ConsoleLogger");
            }
            s_consoleLogger->set_pattern("[%^%l%$] %v");
        }
        catch (const spdlog::spdlog_ex&)
        {
            // If console logger cannot be created, continue; we'll still attempt file logging.
        }

        std::filesystem::path dirPath = resolveLogDir(logDirectory);

        // Create log directory without throwing; fall back to a temp directory if the chosen location isn't writable.
        {
            std::error_code ec;
            std::filesystem::create_directories(dirPath, ec);
            if (ec)
            {
                std::error_code tempEc;
                auto            temp = std::filesystem::temp_directory_path(tempEc);
                if (!tempEc)
                {
                    dirPath = temp / "GameEngineLogs";
                    std::filesystem::create_directories(dirPath, ec);
                }
            }
        }

        // Generate timestamped filename
        std::string filename = generateTimestampedFilename();
        s_currentLogPath     = (dirPath / filename).string();

        // Create basic file sink (no rotation - each session gets a new timestamped file)
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(s_currentLogPath, true);

        // Set log pattern: [timestamp] [level] [thread] message
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [T:%t] %v");

        // Create logger (synchronous on purpose).
        // This avoids any startup deadlocks/hangs related to background logging threads,
        // and ensures the log file is written immediately.
        s_logger = std::make_shared<spdlog::logger>("GameEngineLogger", file_sink);

        // Set log level based on build type.
        // Release should still emit INFO/WARN/ERROR; Debug emits DEBUG too.
#ifndef NDEBUG
        const auto level = spdlog::level::debug;
#else
        const auto level = spdlog::level::info;
#endif
        s_logger->set_level(level);
        if (s_consoleLogger)
        {
            s_consoleLogger->set_level(level);
        }

        // Register the logger (ignore if already registered)
        try
        {
            spdlog::register_logger(s_logger);
        }
        catch (const spdlog::spdlog_ex&)
        {
        }

        // Flush on INFO so logs show up even if the app crashes/hangs.
        s_logger->flush_on(spdlog::level::info);

        s_logger->info("=== Logger Initialized ===");
        s_logger->info("Log file: {}", s_currentLogPath);

        // Force a flush so the file is never empty if initialization succeeded.
        s_logger->flush();

        if (s_consoleLogger)
        {
            s_consoleLogger->info("Logger initialized");
            s_consoleLogger->info("Log file: {}", s_currentLogPath);
            s_consoleLogger->flush();
        }
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
    }
}

void Logger::shutdown()
{
    if (s_logger)
    {
        s_logger->info("=== Logger Shutting Down ===");
        s_logger->flush();
        spdlog::drop("GameEngineLogger");
        spdlog::drop("ConsoleLogger");
        s_logger.reset();
        s_consoleLogger.reset();
        spdlog::shutdown();
    }
}

bool Logger::isInitialized()
{
    return s_logger != nullptr;
}

void Logger::flush()
{
    if (s_logger)
    {
        s_logger->flush();
    }
}

std::string Logger::getCurrentLogPath()
{
    return s_currentLogPath;
}

}  // namespace Internal
