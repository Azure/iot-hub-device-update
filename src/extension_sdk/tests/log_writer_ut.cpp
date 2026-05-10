/**
 * @file log_writer_ut.cpp
 * @brief Unit tests for the binary log writer and log callback.
 */

#include <catch2/catch_all.hpp>

#include <cstdio>
#include <cstring>
#include <string>

extern "C"
{
#include "aduc/log_writer.h"
#include "aduc/log_callback.h"
}

static const char* TEST_LOG_FILE = "test_log_writer.bin";

class TempLogFile
{
public:
    explicit TempLogFile(const char* path = TEST_LOG_FILE) : m_path(path) {}
    ~TempLogFile() { remove(m_path.c_str()); }
    const char* path() const { return m_path.c_str(); }
private:
    std::string m_path;
};

// ─── Log Writer Tests ────────────────────────────────────────────────────────

TEST_CASE("LogWriter: Create log writer succeeds", "[log_writer]")
{
    TempLogFile tmp;
    ADUC_LogWriter* writer = nullptr;

    ADUC_Result2 result = ADUC_Log_Create(tmp.path(), &writer);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(writer != nullptr);

    ADUC_Log_Destroy(writer);
}

TEST_CASE("LogWriter: Write text message does not crash", "[log_writer]")
{
    ADUC_Log_WriteText(ADUC_LOG_INFO, "test", "Hello %s %d", "world", 42);
}

TEST_CASE("LogWriter: Flush does not crash", "[log_writer]")
{
    TempLogFile tmp;
    ADUC_LogWriter* writer = nullptr;

    ADUC_Result2 result = ADUC_Log_Create(tmp.path(), &writer);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    result = ADUC_Log_Flush(writer);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));

    ADUC_Log_Destroy(writer);
}

TEST_CASE("LogWriter: Destroy NULL does not crash", "[log_writer]")
{
    ADUC_Log_Destroy(nullptr); // Should not crash
}

// ─── Log Callback Tests ──────────────────────────────────────────────────────

TEST_CASE("LogCallback: Create with config succeeds", "[log_callback]")
{
    TempLogFile tmp("test_log_callback.bin");
    ADUC_LogWriter* writer = nullptr;
    ADUC_Result2 result = ADUC_Log_Create(tmp.path(), &writer);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    ADUC_LogCallbackConfig config = {};
    config.globalMinLevel = ADUC_LOG_INFO;
    config.mirrorToStderr = false;
    config.includeTimestamp = true;
    config.timestampFormat = "%Y-%m-%d %H:%M:%S";

    ADUC_LogCallbackContext* ctx = nullptr;
    result = ADUC_LogCallback_Create(&config, writer, &ctx);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(ctx != nullptr);

    ADUC_LogCallback_Destroy(ctx);
    ADUC_Log_Destroy(writer);
}

TEST_CASE("LogCallback: Filter by level", "[log_callback]")
{
    TempLogFile tmp("test_log_filter.bin");
    ADUC_LogWriter* writer = nullptr;
    ADUC_Result2 result = ADUC_Log_Create(tmp.path(), &writer);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    ADUC_LogCallbackConfig config = {};
    config.globalMinLevel = ADUC_LOG_WARN; // Only WARN and above
    config.mirrorToStderr = false;

    ADUC_LogCallbackContext* ctx = nullptr;
    result = ADUC_LogCallback_Create(&config, writer, &ctx);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    // Set min level to WARN - INFO messages should be filtered
    ADUC_LogCallback_SetMinLevel(ctx, ADUC_LOG_WARN);

    // Get bytes before logging
    size_t bytesBefore = ADUC_Log_GetBytesWritten(writer);

    // Get the log function and call it with INFO level - should be filtered
    ADUC_LogFn logFn = ADUC_LogCallback_GetLogFn();
    REQUIRE(logFn != nullptr);

    void* logCtx = ADUC_LogCallback_GetContext(ctx);
    logFn(logCtx, ADUC_LOG_INFO, "test", 1001, "This should be filtered: %s", "info msg");

    ADUC_LogCallback_Flush(ctx);
    size_t bytesAfter = ADUC_Log_GetBytesWritten(writer);

    // INFO is more verbose than WARN, so it should be filtered out
    CHECK(bytesAfter == bytesBefore);

    ADUC_LogCallback_Destroy(ctx);
    ADUC_Log_Destroy(writer);
}

TEST_CASE("LogCallback: stderr mirror works", "[log_callback]")
{
    TempLogFile tmp("test_log_stderr.bin");
    ADUC_LogWriter* writer = nullptr;
    ADUC_Result2 result = ADUC_Log_Create(tmp.path(), &writer);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    ADUC_LogCallbackConfig config = {};
    config.globalMinLevel = ADUC_LOG_DEBUG;
    config.mirrorToStderr = true;
    config.includeTimestamp = true;

    ADUC_LogCallbackContext* ctx = nullptr;
    result = ADUC_LogCallback_Create(&config, writer, &ctx);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));

    // Just verify it doesn't crash when mirroring to stderr
    ADUC_LogFn logFn = ADUC_LogCallback_GetLogFn();
    REQUIRE(logFn != nullptr);
    void* logCtx = ADUC_LogCallback_GetContext(ctx);
    logFn(logCtx, ADUC_LOG_WARN, "test", 1002, "Mirror test message");

    ADUC_LogCallback_Destroy(ctx);
    ADUC_Log_Destroy(writer);
}

TEST_CASE("LogCallback: Per-component level override", "[log_callback]")
{
    TempLogFile tmp("test_log_component.bin");
    ADUC_LogWriter* writer = nullptr;
    ADUC_Result2 result = ADUC_Log_Create(tmp.path(), &writer);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    // Set the binary writer's level to DEBUG so it doesn't filter the entry
    ADUC_Log_SetLevel(ADUC_LOG_DEBUG);

    ADUC_LogCallbackConfig config = {};
    config.globalMinLevel = ADUC_LOG_WARN; // Global: WARN
    config.mirrorToStderr = false;

    ADUC_LogCallbackContext* ctx = nullptr;
    result = ADUC_LogCallback_Create(&config, writer, &ctx);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    // Override: allow DEBUG for "downloader" component
    ADUC_LogCallback_SetComponentLevel(ctx, "downloader", ADUC_LOG_DEBUG);

    ADUC_LogFn logFn = ADUC_LogCallback_GetLogFn();
    void* logCtx = ADUC_LogCallback_GetContext(ctx);

    size_t bytesBefore = ADUC_Log_GetBytesWritten(writer);

    // DEBUG message from "downloader" should pass (component override)
    logFn(logCtx, ADUC_LOG_DEBUG, "downloader", 1003, "Debug from downloader");
    ADUC_LogCallback_Flush(ctx);

    size_t bytesAfter = ADUC_Log_GetBytesWritten(writer);
    CHECK(bytesAfter > bytesBefore);

    ADUC_LogCallback_Destroy(ctx);
    ADUC_Log_SetLevel(ADUC_LOG_INFO); // Restore default
    ADUC_Log_Destroy(writer);
}

TEST_CASE("LogCallback: Destroy NULL does not crash", "[log_callback]")
{
    ADUC_LogCallback_Destroy(nullptr); // Should not crash
}
