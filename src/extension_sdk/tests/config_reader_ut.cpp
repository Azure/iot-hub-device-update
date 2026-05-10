/**
 * @file config_reader_ut.cpp
 * @brief Unit tests for the TOML-based config reader.
 */

#include <catch2/catch_all.hpp>

#include <cstdio>
#include <cstring>
#include <string>

extern "C"
{
#include "aduc/config_reader.h"
#include "aduc/extension_types.h"
}

// Helper RAII wrapper that writes a TOML string to a temp file and removes it on destruction.
class TempTomlFile
{
public:
    explicit TempTomlFile(const char* content, const char* name = "test_config.toml")
        : m_path(name)
    {
        FILE* f = fopen(m_path.c_str(), "w");
        REQUIRE(f != nullptr);
        fputs(content, f);
        fclose(f);
    }

    ~TempTomlFile()
    {
        remove(m_path.c_str());
    }

    const char* path() const
    {
        return m_path.c_str();
    }

private:
    std::string m_path;
};

TEST_CASE("ConfigReader: Load valid TOML file", "[config_reader]")
{
    const char* toml = R"(
[agent]
device_id = "test-device-001"
manufacturer = "Contoso"

[logging]
min_level = "info"
max_file_size = 65536
verbose = true
)";

    TempTomlFile file(toml);
    ADUC_ConfigHandle handle = nullptr;

    ADUC_Result2 result = ADUC_Config_LoadFile(file.path(), &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    REQUIRE(handle != nullptr);

    ADUC_Config_Free(handle);
}

TEST_CASE("ConfigReader: GetString for known keys", "[config_reader]")
{
    const char* toml = R"(
[agent]
device_id = "my-device"
manufacturer = "Fabrikam"
)";

    TempTomlFile file(toml);
    ADUC_ConfigHandle handle = nullptr;
    ADUC_Result2 result = ADUC_Config_LoadFile(file.path(), &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    const char* deviceId = ADUC_Config_GetString(handle, "agent.device_id");
    REQUIRE(deviceId != nullptr);
    CHECK(std::string(deviceId) == "my-device");

    const char* mfr = ADUC_Config_GetString(handle, "agent.manufacturer");
    REQUIRE(mfr != nullptr);
    CHECK(std::string(mfr) == "Fabrikam");

    // Non-existent key returns NULL
    CHECK(ADUC_Config_GetString(handle, "agent.nonexistent") == nullptr);

    ADUC_Config_Free(handle);
}

TEST_CASE("ConfigReader: GetInt for known keys", "[config_reader]")
{
    const char* toml = R"(
[logging]
max_file_size = 65536
retry_count = 3
)";

    TempTomlFile file(toml);
    ADUC_ConfigHandle handle = nullptr;
    ADUC_Result2 result = ADUC_Config_LoadFile(file.path(), &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    int64_t val = 0;
    result = ADUC_Config_GetInt(handle, "logging.max_file_size", &val);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(val == 65536);

    result = ADUC_Config_GetInt(handle, "logging.retry_count", &val);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(val == 3);

    // Non-existent key → failure
    result = ADUC_Config_GetInt(handle, "logging.nonexistent", &val);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    ADUC_Config_Free(handle);
}

TEST_CASE("ConfigReader: GetBool for known keys", "[config_reader]")
{
    const char* toml = R"(
[logging]
verbose = true
compact = false
)";

    TempTomlFile file(toml);
    ADUC_ConfigHandle handle = nullptr;
    ADUC_Result2 result = ADUC_Config_LoadFile(file.path(), &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    bool val = false;
    result = ADUC_Config_GetBool(handle, "logging.verbose", &val);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(val == true);

    result = ADUC_Config_GetBool(handle, "logging.compact", &val);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(val == false);

    // Non-existent key → failure
    result = ADUC_Config_GetBool(handle, "logging.nonexistent", &val);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    ADUC_Config_Free(handle);
}

TEST_CASE("ConfigReader: HasKey", "[config_reader]")
{
    const char* toml = R"(
[agent]
device_id = "dev1"
)";

    TempTomlFile file(toml);
    ADUC_ConfigHandle handle = nullptr;
    ADUC_Result2 result = ADUC_Config_LoadFile(file.path(), &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    CHECK(ADUC_Config_HasKey(handle, "agent.device_id") == true);
    CHECK(ADUC_Config_HasKey(handle, "agent.nonexistent") == false);

    ADUC_Config_Free(handle);
}

TEST_CASE("ConfigReader: Missing file returns failure", "[config_reader]")
{
    ADUC_ConfigHandle handle = nullptr;
    ADUC_Result2 result = ADUC_Config_LoadFile("/no/such/file.toml", &handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
    CHECK(handle == nullptr);
}

TEST_CASE("ConfigReader: NULL parameters return failure", "[config_reader]")
{
    ADUC_ConfigHandle handle = nullptr;

    // NULL filePath
    ADUC_Result2 result = ADUC_Config_LoadFile(nullptr, &handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    // NULL outHandle
    result = ADUC_Config_LoadFile("dummy.toml", nullptr);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

TEST_CASE("ConfigReader: Invalid TOML content", "[config_reader]")
{
    const char* badToml = "this is not valid [[[ toml content !!!";

    TempTomlFile file(badToml, "bad_config.toml");
    ADUC_ConfigHandle handle = nullptr;
    ADUC_Result2 result = ADUC_Config_LoadFile(file.path(), &handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

TEST_CASE("ConfigReader: Free NULL handle is safe", "[config_reader]")
{
    ADUC_Config_Free(nullptr); // Should not crash
}

TEST_CASE("ConfigReader: GetString on NULL handle returns NULL", "[config_reader]")
{
    CHECK(ADUC_Config_GetString(nullptr, "some.key") == nullptr);
}

TEST_CASE("ConfigReader: Nested dotted key navigation", "[config_reader]")
{
    const char* toml = R"(
[workflow.retry]
max_retries = 5
delay_ms = 1000
)";

    TempTomlFile file(toml);
    ADUC_ConfigHandle handle = nullptr;
    ADUC_Result2 result = ADUC_Config_LoadFile(file.path(), &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    int64_t val = 0;
    result = ADUC_Config_GetInt(handle, "workflow.retry.max_retries", &val);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(val == 5);

    ADUC_Config_Free(handle);
}
