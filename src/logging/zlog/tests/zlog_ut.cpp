#include <aduc/logging.h>
#include <catch2/catch_all.hpp>
#include <zlog.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

extern "C" {
ADUC_LOG_SEVERITY ZLogLevelToAducLogSeverity(enum ZLOG_SEVERITY logLevel);
void zlog_ensure_at_most_n_logfiles(int max_num);
}

namespace
{
std::filesystem::path MakeUniqueTempDir(const std::string& suffix)
{
    const auto nonce = static_cast<long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::path dir =
        std::filesystem::temp_directory_path() / ("zlog-ut-" + suffix + "-" + std::to_string(nonce));
    std::filesystem::create_directories(dir);
    return dir;
}

std::filesystem::path FindFirstLogFile(const std::filesystem::path& dir, const std::string& prefix)
{
    for (const auto& entry : std::filesystem::directory_iterator(dir))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const std::string name = entry.path().filename().string();
        if (name.find(prefix + ".") == 0 && entry.path().extension() == ".log")
        {
            return entry.path();
        }
    }

    return std::filesystem::path();
}

size_t CountLogFiles(const std::filesystem::path& dir, const std::string& prefix)
{
    size_t count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const std::string name = entry.path().filename().string();
        if (name.find(prefix + ".") == 0 && entry.path().extension() == ".log")
        {
            ++count;
        }
    }

    return count;
}
} // namespace

TEST_CASE("ADUC_Logging_Init handles refcount and level")
{
    ADUC_Logging_Init(ADUC_LOG_INFO, "zlog-ut-refcount");
    CHECK(ADUC_Logging_GetLevel() == ADUC_LOG_INFO);

    ADUC_Logging_Init(ADUC_LOG_DEBUG, "zlog-ut-refcount");
    CHECK(ADUC_Logging_GetLevel() == ADUC_LOG_INFO);

    ADUC_Logging_Uninit();
    ADUC_Logging_Uninit();
}

TEST_CASE("zlog supports disabled and file-backed logging")
{
    SECTION("disabled console and file returns success and logs are ignored")
    {
        REQUIRE(zlog_init(".", "zlog-ut-disabled", ZLOG_DISABLED, ZLOG_DISABLED, ZLOG_DEBUG, ZLOG_DEBUG) == 0);
        zlog_log(ZLOG_INFO, __FUNCTION__, __LINE__, "this is intentionally ignored");
        zlog_finish();
    }

    SECTION("file logging writes one-line and multi-line records")
    {
        std::filesystem::path logDir = MakeUniqueTempDir("file");
        const std::string prefix = "zlog-ut-file";

        REQUIRE(zlog_init(logDir.string().c_str(), prefix.c_str(), ZLOG_DISABLED, ZLOG_ENABLED, ZLOG_DEBUG, ZLOG_DEBUG) == 0);

        zlog_log(ZLOG_INFO, __FUNCTION__, __LINE__, "short message");

        std::string longMessage(600, 'A');
        zlog_log(ZLOG_WARN, __FUNCTION__, __LINE__, "%s", longMessage.c_str());

        zlog_flush_buffer();
        zlog_finish();

        std::filesystem::path logfile = FindFirstLogFile(logDir, prefix);
        REQUIRE_FALSE(logfile.empty());

        std::ifstream in(logfile, std::ios::in);
        REQUIRE(in.is_open());

        const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        CHECK(content.find("short message") != std::string::npos);
        CHECK(content.find("MULTI-LINE LOG BEGIN") != std::string::npos);
    }
}

TEST_CASE("zlog severity mappings cover all branches")
{
    CHECK(ZLogLevelToAducLogSeverity(ZLOG_DEBUG) == ADUC_LOG_DEBUG);
    CHECK(ZLogLevelToAducLogSeverity(ZLOG_INFO) == ADUC_LOG_INFO);
    CHECK(ZLogLevelToAducLogSeverity(ZLOG_WARN) == ADUC_LOG_WARN);
    CHECK(ZLogLevelToAducLogSeverity(ZLOG_ERROR) == ADUC_LOG_ERROR);

    ADUC_Logging_Init(ADUC_LOG_DEBUG, "zlog-ut-map");
    CHECK(ADUC_Logging_GetLevel() == ADUC_LOG_DEBUG);
    ADUC_Logging_Uninit();

    ADUC_Logging_Init(ADUC_LOG_INFO, "zlog-ut-map");
    CHECK(ADUC_Logging_GetLevel() == ADUC_LOG_INFO);
    ADUC_Logging_Uninit();

    ADUC_Logging_Init(ADUC_LOG_WARN, "zlog-ut-map");
    CHECK(ADUC_Logging_GetLevel() == ADUC_LOG_WARN);
    ADUC_Logging_Uninit();

    ADUC_Logging_Init(ADUC_LOG_ERROR, "zlog-ut-map");
    CHECK(ADUC_Logging_GetLevel() == ADUC_LOG_ERROR);
    ADUC_Logging_Uninit();
}

TEST_CASE("zlog file retention deletes older matching logs")
{
    std::filesystem::path logDir = MakeUniqueTempDir("retention");
    const std::string prefix = "zlog-ut-retention";

    REQUIRE(zlog_init(logDir.string().c_str(), prefix.c_str(), ZLOG_DISABLED, ZLOG_ENABLED, ZLOG_DEBUG, ZLOG_DEBUG) == 0);
    zlog_log(ZLOG_INFO, __FUNCTION__, __LINE__, "seed file");
    zlog_flush_buffer();
    zlog_finish();

    for (int i = 0; i < 4; ++i)
    {
        std::filesystem::path fake = logDir / (prefix + ".19990101-00000" + std::to_string(i) + ".log");
        std::ofstream out(fake, std::ios::out | std::ios::trunc);
        REQUIRE(out.is_open());
        out << "old";
    }

    REQUIRE(zlog_init(logDir.string().c_str(), prefix.c_str(), ZLOG_DISABLED, ZLOG_ENABLED, ZLOG_DEBUG, ZLOG_DEBUG) == 0);
    zlog_ensure_at_most_n_logfiles(1);
    zlog_finish();

    CHECK(CountLogFiles(logDir, prefix) <= 1);
}
