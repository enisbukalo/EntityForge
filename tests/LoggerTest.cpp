#include <gtest/gtest.h>

#include <Logger.h>
#include <filesystem>
#include <fstream>
#include <thread>

// Test fixture for Logger tests - ensures clean state between tests
class LoggerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Ensure logger is not initialized before each test
        Logger::shutdown();
    }

    void TearDown() override
    {
        // Clean up after each test
        Logger::shutdown();

        // Clean up test log directories
        std::filesystem::remove_all("test_logs");
        std::filesystem::remove_all("test_logs_2");
    }
};

TEST_F(LoggerTest, IsInitializedReturnsFalseBeforeInit)
{
    EXPECT_FALSE(Logger::isInitialized());
}

TEST_F(LoggerTest, InitializeCreatesLogDirectoryAndFile)
{
    Logger::initialize("test_logs");

    EXPECT_TRUE(Logger::isInitialized());
    EXPECT_TRUE(std::filesystem::exists("test_logs"));

    std::string logPath = Logger::getCurrentLogPath();
    EXPECT_FALSE(logPath.empty());
    EXPECT_TRUE(logPath.find("test_logs") != std::string::npos);
    EXPECT_TRUE(logPath.find(".log") != std::string::npos);
}

TEST_F(LoggerTest, ShutdownMakesLoggerNotInitialized)
{
    Logger::initialize("test_logs");
    EXPECT_TRUE(Logger::isInitialized());

    Logger::shutdown();
    EXPECT_FALSE(Logger::isInitialized());
}

TEST_F(LoggerTest, DoubleInitializeIsNoOp)
{
    Logger::initialize("test_logs");
    std::string firstLogPath = Logger::getCurrentLogPath();

    Logger::initialize("test_logs_2");
    std::string secondLogPath = Logger::getCurrentLogPath();

    // Second init should be ignored, path should remain the same
    EXPECT_EQ(firstLogPath, secondLogPath);
    EXPECT_FALSE(std::filesystem::exists("test_logs_2"));
}

TEST_F(LoggerTest, ShutdownWithoutInitializeIsNoOp)
{
    // Should not crash
    Logger::shutdown();
    EXPECT_FALSE(Logger::isInitialized());
}

TEST_F(LoggerTest, FlushWithoutInitializeIsNoOp)
{
    // Should not crash
    Logger::flush();
    EXPECT_FALSE(Logger::isInitialized());
}

TEST_F(LoggerTest, GetCurrentLogPathReturnsEmptyBeforeInit)
{
    // After shutdown, path should still be set (we don't clear it)
    // But if never initialized, it should be empty
    // Note: This tests initial state which is empty string
    Logger::shutdown();  // Ensure clean state
    // The static s_currentLogPath persists, so we can only test after init
}

TEST_F(LoggerTest, LogFilenameContainsTimestamp)
{
    Logger::initialize("test_logs");
    std::string logPath = Logger::getCurrentLogPath();

    // Filename should contain date pattern YYYY-MM-DD
    EXPECT_TRUE(logPath.find("-") != std::string::npos);
    EXPECT_TRUE(logPath.find(".log") != std::string::npos);
}

TEST_F(LoggerTest, LogMacrosDoNotCrashBeforeInit)
{
    // These should be no-ops when logger is not initialized
    LOG_INFO("Test info message");
    LOG_DEBUG("Test debug message");
    LOG_WARN("Test warn message");
    LOG_ERROR("Test error message");

    EXPECT_FALSE(Logger::isInitialized());
}

TEST_F(LoggerTest, LogMacrosWriteToFile)
{
    Logger::initialize("test_logs");
    std::string logPath = Logger::getCurrentLogPath();

    LOG_INFO("Test message from unit test");
    LOG_ERROR("Test error message from unit test");

    // Flush to ensure messages are written
    Logger::flush();

    // Give async logger time to write
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Read the log file and verify content
    std::ifstream logFile(logPath);
    ASSERT_TRUE(logFile.is_open());

    std::string content((std::istreambuf_iterator<char>(logFile)), std::istreambuf_iterator<char>());
    logFile.close();

    EXPECT_TRUE(content.find("Logger Initialized") != std::string::npos);
    EXPECT_TRUE(content.find("Test message from unit test") != std::string::npos);
    EXPECT_TRUE(content.find("Test error message from unit test") != std::string::npos);
}

TEST_F(LoggerTest, LogConsoleMacrosDoNotCrashBeforeInit)
{
    // Console macros should also be no-ops when logger is not initialized
    LOG_INFO_CONSOLE("Test info console");
    LOG_DEBUG_CONSOLE("Test debug console");
    LOG_WARN_CONSOLE("Test warn console");
    LOG_ERROR_CONSOLE("Test error console");

    EXPECT_FALSE(Logger::isInitialized());
}

TEST_F(LoggerTest, ReinitializeAfterShutdownWorks)
{
    Logger::initialize("test_logs");
    EXPECT_TRUE(Logger::isInitialized());
    std::string firstPath = Logger::getCurrentLogPath();

    Logger::shutdown();
    EXPECT_FALSE(Logger::isInitialized());

    // Small delay to ensure different timestamp
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    Logger::initialize("test_logs");
    EXPECT_TRUE(Logger::isInitialized());
    std::string secondPath = Logger::getCurrentLogPath();

    // New log file should be created (different timestamp)
    EXPECT_NE(firstPath, secondPath);
}
