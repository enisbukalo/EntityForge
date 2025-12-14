#include "Logger.h"
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>

namespace Internal
{

std::shared_ptr<spdlog::logger> Logger::s_logger         = nullptr;
std::shared_ptr<spdlog::logger> Logger::s_consoleLogger  = nullptr;
std::string                     Logger::s_currentLogPath = "";

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
        // Create log directory if it doesn't exist
        std::filesystem::create_directories(logDirectory);

        // Generate timestamped filename
        std::string filename = generateTimestampedFilename();
        s_currentLogPath     = logDirectory + "/" + filename;

        // Initialize spdlog thread pool for async logging
        // Queue size: 8192, Thread count: 1
        spdlog::init_thread_pool(8192, 1);

        // Create basic file sink (no rotation - each session gets a new timestamped file)
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(s_currentLogPath, true);

        // Set log pattern: [timestamp] [level] [thread] message
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [T:%t] %v");

        // Create async logger with the file sink
        s_logger = std::make_shared<spdlog::async_logger>("GameEngineLogger",
                                                          file_sink,
                                                          spdlog::thread_pool(),
                                                          spdlog::async_overflow_policy::overrun_oldest);

        // Create console logger for optional console output (used by game developers)
        s_consoleLogger = spdlog::stdout_color_mt("ConsoleLogger");
        s_consoleLogger->set_pattern("[%^%l%$] %v");

        // Set log level based on build type
#ifndef NDEBUG
        s_logger->set_level(spdlog::level::debug);
        s_consoleLogger->set_level(spdlog::level::debug);
#else
        s_logger->set_level(spdlog::level::err);
        s_consoleLogger->set_level(spdlog::level::err);
#endif

        // Register the logger
        spdlog::register_logger(s_logger);

        // Flush on error level and above
        s_logger->flush_on(spdlog::level::err);

        s_logger->info("=== Logger Initialized ===");
        s_logger->info("Log file: {}", s_currentLogPath);
    }
    catch (const spdlog::spdlog_ex& ex)
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
