/**
 * @file agent_main_ut.cpp
 * @brief Unit tests for agent main lifecycle: initialization, config loading,
 *        extension loading, poll cycle, and shutdown cleanup.
 *
 * Note: ADUC_Agent_ParseArgs and ADUC_Agent_Run live in the daemon binary
 * (agent_main.c contains main()), not the SDK static library.  We test the
 * underlying subsystems that agent_main.c depends on: config reader,
 * extension registry, and the AgentConfig structure contract.
 */

#include <catch2/catch_all.hpp>

#include <cstdio>
#include <cstring>
#include <string>

extern "C"
{
#include "aduc/config_reader.h"
#include "aduc/extension_loader.h"
#include "aduc/extension_types.h"
}

// ─── Helper: RAII temp file ──────────────────────────────────────────────────

class TempFile
{
public:
    explicit TempFile(const char* content, const char* name)
        : m_path(name)
    {
        FILE* f = fopen(m_path.c_str(), "w");
        REQUIRE(f != nullptr);
        fputs(content, f);
        fclose(f);
    }

    ~TempFile()
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

// ─── Config Loading Tests ────────────────────────────────────────────────────

TEST_CASE("Config: Loads from TOML file successfully", "[agent][config]")
{
    const char* toml = R"(
[agent]
device_id = "test-device"
manufacturer = "TestCo"

[extensions]
search_path = "/usr/lib/adu/extensions"

[communication]
endpoint = "https://test.azure.com"
poll_interval_sec = 60
)";

    TempFile file(toml, "agent_test_config.toml");
    ADUC_ConfigHandle handle = nullptr;
    ADUC_Result2 result = ADUC_Config_LoadFile(file.path(), &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    REQUIRE(handle != nullptr);

    const char* deviceId = ADUC_Config_GetString(handle, "agent.device_id");
    REQUIRE(deviceId != nullptr);
    CHECK(std::string(deviceId) == "test-device");

    const char* searchPath = ADUC_Config_GetString(handle, "extensions.search_path");
    REQUIRE(searchPath != nullptr);
    CHECK(std::string(searchPath) == "/usr/lib/adu/extensions");

    int64_t pollInterval = 0;
    result = ADUC_Config_GetInt(handle, "communication.poll_interval_sec", &pollInterval);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(pollInterval == 60);

    ADUC_Config_Free(handle);
}

TEST_CASE("Config: Validates required fields are present", "[agent][config]")
{
    const char* toml = R"(
[agent]
device_id = "dev1"
)";

    TempFile file(toml, "agent_validate_config.toml");
    ADUC_ConfigHandle handle = nullptr;
    ADUC_Result2 result = ADUC_Config_LoadFile(file.path(), &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    // Required field exists
    CHECK(ADUC_Config_HasKey(handle, "agent.device_id") == true);

    // Optional fields that may not exist
    CHECK(ADUC_Config_HasKey(handle, "extensions.search_path") == false);
    CHECK(ADUC_Config_HasKey(handle, "communication.endpoint") == false);

    ADUC_Config_Free(handle);
}

TEST_CASE("Config: Handles missing optional fields gracefully", "[agent][config]")
{
    const char* toml = R"(
[agent]
device_id = "minimal-device"
)";

    TempFile file(toml, "agent_minimal_config.toml");
    ADUC_ConfigHandle handle = nullptr;
    ADUC_Result2 result = ADUC_Config_LoadFile(file.path(), &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    // GetString for missing key returns NULL (not crash)
    CHECK(ADUC_Config_GetString(handle, "logging.min_level") == nullptr);
    CHECK(ADUC_Config_GetString(handle, "communication.endpoint") == nullptr);

    // GetInt for missing key returns failure (not crash)
    int64_t val = 0;
    result = ADUC_Config_GetInt(handle, "communication.poll_interval_sec", &val);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    // GetBool for missing key returns failure
    bool bval = false;
    result = ADUC_Config_GetBool(handle, "logging.verbose", &bval);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    ADUC_Config_Free(handle);
}

TEST_CASE("Config: Layering — factory + user override values", "[agent][config]")
{
    // Simulate layered config by loading a file with overridden values
    const char* toml = R"(
[agent]
device_id = "overridden-device"

[workflow.retry]
max_retries = 10
delay_ms = 2000
)";

    TempFile file(toml, "agent_layered_config.toml");
    ADUC_ConfigHandle handle = nullptr;
    ADUC_Result2 result = ADUC_Config_LoadFile(file.path(), &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    // Verify overridden values
    const char* deviceId = ADUC_Config_GetString(handle, "agent.device_id");
    REQUIRE(deviceId != nullptr);
    CHECK(std::string(deviceId) == "overridden-device");

    int64_t retries = 0;
    result = ADUC_Config_GetInt(handle, "workflow.retry.max_retries", &retries);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(retries == 10);

    int64_t delay = 0;
    result = ADUC_Config_GetInt(handle, "workflow.retry.delay_ms", &delay);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(delay == 2000);

    ADUC_Config_Free(handle);
}

// ─── Extension Loading Tests ─────────────────────────────────────────────────

TEST_CASE("ExtensionLoading: Load non-existent .so fails", "[agent][extension]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    result = ADUC_ExtensionRegistry_LoadFromFile(registry, "/no/such/handler.so", nullptr);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
    CHECK(ADUC_ExtensionRegistry_GetCount(registry) == 0);

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("ExtensionLoading: Registry lookup by type on empty returns NULL", "[agent][extension]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    CHECK(ADUC_ExtensionRegistry_FindByType(registry, ADUC_EXT_TYPE_STEP_HANDLER, 0) == nullptr);
    CHECK(ADUC_ExtensionRegistry_FindByType(registry, ADUC_EXT_TYPE_COMMUNICATION, 0) == nullptr);
    CHECK(ADUC_ExtensionRegistry_FindByType(registry, ADUC_EXT_TYPE_DOWNLOADER, 0) == nullptr);

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("ExtensionLoading: Registry lookup by capability on empty returns NULL", "[agent][extension]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    CHECK(ADUC_ExtensionRegistry_FindByCapability(registry, "microsoft/script:2") == nullptr);
    CHECK(ADUC_ExtensionRegistry_FindByCapability(registry, "microsoft/apt:2") == nullptr);
    CHECK(ADUC_ExtensionRegistry_FindByCapability(registry, "microsoft/swupdate:2") == nullptr);

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("ExtensionLoading: FindAllByType returns NULL with count 0 on empty registry", "[agent][extension]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    size_t count = 99;
    const ADUC_ExtensionDescriptor** descs =
        ADUC_ExtensionRegistry_FindAllByType(registry, ADUC_EXT_TYPE_STEP_HANDLER, &count);

    CHECK(count == 0);
    CHECK(descs == nullptr);

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("ExtensionLoading: GetCountByType returns 0 for each type on empty registry", "[agent][extension]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    CHECK(ADUC_ExtensionRegistry_GetCountByType(registry, ADUC_EXT_TYPE_STEP_HANDLER) == 0);
    CHECK(ADUC_ExtensionRegistry_GetCountByType(registry, ADUC_EXT_TYPE_COMMUNICATION) == 0);
    CHECK(ADUC_ExtensionRegistry_GetCountByType(registry, ADUC_EXT_TYPE_DOWNLOADER) == 0);
    CHECK(ADUC_ExtensionRegistry_GetCountByType(registry, ADUC_EXT_TYPE_CONTENT_PROCESSOR) == 0);
    CHECK(ADUC_ExtensionRegistry_GetCountByType(registry, ADUC_EXT_TYPE_COMPONENT_ENUMERATOR) == 0);
    CHECK(ADUC_ExtensionRegistry_GetCountByType(registry, ADUC_EXT_TYPE_AUTHENTICATOR) == 0);

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("ExtensionLoading: ScanDirectory with non-existent dir fails", "[agent][extension]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    result = ADUC_ExtensionRegistry_ScanDirectory(registry, "/nonexistent/extension/dir", nullptr);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
    CHECK(ADUC_ExtensionRegistry_GetCount(registry) == 0);

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("ExtensionLoading: Registry handles no extensions gracefully", "[agent][extension]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    // With no extensions loaded, all queries return safely
    CHECK(ADUC_ExtensionRegistry_GetCount(registry) == 0);
    CHECK(ADUC_ExtensionRegistry_FindById(registry, "any-id") == nullptr);
    CHECK(ADUC_ExtensionRegistry_FindByType(registry, ADUC_EXT_TYPE_COMMUNICATION, 0) == nullptr);
    CHECK(ADUC_ExtensionRegistry_FindByCapability(registry, "any/cap:1") == nullptr);

    // Destroying empty registry is safe
    ADUC_ExtensionRegistry_Destroy(registry);
}

// ─── Agent Lifecycle: shutdown cleanup ───────────────────────────────────────

TEST_CASE("Agent: Shutdown cleanup — Destroy NULL registry is safe", "[agent][lifecycle]")
{
    ADUC_ExtensionRegistry_Destroy(nullptr);
    // No crash = pass
}

TEST_CASE("Agent: Shutdown cleanup — Free NULL config is safe", "[agent][lifecycle]")
{
    ADUC_Config_Free(nullptr);
    // No crash = pass
}

TEST_CASE("Agent: Shutdown cleanup — create and destroy registry", "[agent][lifecycle]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    REQUIRE(registry != nullptr);

    // Simulate loading nothing, then cleanup
    ADUC_ExtensionRegistry_Destroy(registry);
    // No crash = pass
}

TEST_CASE("Agent: Shutdown cleanup — create and free config", "[agent][lifecycle]")
{
    const char* toml = R"(
[agent]
device_id = "cleanup-test"
)";

    TempFile file(toml, "agent_cleanup_config.toml");
    ADUC_ConfigHandle handle = nullptr;
    ADUC_Result2 result = ADUC_Config_LoadFile(file.path(), &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    // Free should release all memory
    ADUC_Config_Free(handle);
    // No crash = pass
}

// ─── Agent config structure contract tests ───────────────────────────────────
// These verify the types and constants the agent daemon depends on.

TEST_CASE("Agent: Log levels have expected values", "[agent][config]")
{
    CHECK(ADUC_LOG_FATAL == 1);
    CHECK(ADUC_LOG_ERROR == 2);
    CHECK(ADUC_LOG_WARN == 3);
    CHECK(ADUC_LOG_INFO == 4);
    CHECK(ADUC_LOG_DEBUG == 5);
    CHECK(ADUC_LOG_TRACE == 6);
}

TEST_CASE("Agent: Extension types are distinct", "[agent][config]")
{
    CHECK(ADUC_EXT_TYPE_COMMUNICATION != ADUC_EXT_TYPE_STEP_HANDLER);
    CHECK(ADUC_EXT_TYPE_STEP_HANDLER != ADUC_EXT_TYPE_CONTENT_PROCESSOR);
    CHECK(ADUC_EXT_TYPE_CONTENT_PROCESSOR != ADUC_EXT_TYPE_DOWNLOADER);
    CHECK(ADUC_EXT_TYPE_DOWNLOADER != ADUC_EXT_TYPE_COMPONENT_ENUMERATOR);
    CHECK(ADUC_EXT_TYPE_COMPONENT_ENUMERATOR != ADUC_EXT_TYPE_AUTHENTICATOR);
}

TEST_CASE("Agent: ADUC_Result2 success and failure macros", "[agent][config]")
{
    ADUC_Result2 success = ADUC_RESULT2_SUCCESS;
    CHECK(ADUC_RESULT2_IS_SUCCESS(success));
    CHECK_FALSE(ADUC_RESULT2_IS_FAILURE(success));

    ADUC_Result2 failure = ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 1);
    CHECK(ADUC_RESULT2_IS_FAILURE(failure));
    CHECK_FALSE(ADUC_RESULT2_IS_SUCCESS(failure));
}

TEST_CASE("Agent: Outcome enum values and string conversion", "[agent][config]")
{
    CHECK(std::string(ADUC_Outcome_ToString(ADUC_Outcome_Succeeded)) == "SUCCEEDED");
    CHECK(std::string(ADUC_Outcome_ToString(ADUC_Outcome_Failed)) == "FAILED");
    CHECK(std::string(ADUC_Outcome_ToString(ADUC_Outcome_Canceled)) == "CANCELED");
    CHECK(std::string(ADUC_Outcome_ToString(ADUC_Outcome_Skipped)) == "SKIPPED");
}

TEST_CASE("Agent: FailureOrigin enum values and string conversion", "[agent][config]")
{
    CHECK(std::string(ADUC_FailureOrigin_ToString(ADUC_FailureOrigin_NotApplicable)) == "NOT_APPLICABLE");
    CHECK(std::string(ADUC_FailureOrigin_ToString(ADUC_FailureOrigin_AduCloudService)) == "ADU_CLOUD_SERVICE");
    CHECK(std::string(ADUC_FailureOrigin_ToString(ADUC_FailureOrigin_AduManagedResource)) == "ADU_MANAGED_RESOURCE");
    CHECK(std::string(ADUC_FailureOrigin_ToString(ADUC_FailureOrigin_AgentCore)) == "AGENT_CORE");
    CHECK(std::string(ADUC_FailureOrigin_ToString(ADUC_FailureOrigin_AgentExtension)) == "AGENT_EXTENSION");
    CHECK(std::string(ADUC_FailureOrigin_ToString(ADUC_FailureOrigin_AgentDependency)) == "AGENT_DEPENDENCY");
    CHECK(std::string(ADUC_FailureOrigin_ToString(ADUC_FailureOrigin_Device)) == "DEVICE");
    // Backward compat aliases still produce correct v3 strings
    CHECK(std::string(ADUC_Origin_ToString(ADUC_Origin_AduService)) == "ADU_CLOUD_SERVICE");
    CHECK(std::string(ADUC_Origin_ToString(ADUC_Origin_AgentCore)) == "AGENT_CORE");
}
