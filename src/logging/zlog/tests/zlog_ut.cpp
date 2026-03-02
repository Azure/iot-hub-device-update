#include <aduc/logging.h>
#include <catch2/catch_all.hpp>
#include <zlog.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

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
