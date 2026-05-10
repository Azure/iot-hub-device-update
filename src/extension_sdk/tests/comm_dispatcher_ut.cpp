/**
 * @file comm_dispatcher_ut.cpp
 * @brief Unit tests for the communication dispatcher.
 */

#include <catch2/catch_all.hpp>

extern "C"
{
#include "aduc/comm_dispatcher.h"
#include "aduc/extension_loader.h"
}

TEST_CASE("CommDispatcher: Create with empty config fails", "[comm_dispatcher]")
{
    ADUC_CommDispatcherConfig config = {};
    config.providers = nullptr;
    config.providerCount = 0;
    config.failoverTimeoutMs = 5000;

    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    ADUC_CommDispatcherHandle handle = nullptr;
    result = ADUC_CommDispatcher_Create(&config, registry, &handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
    CHECK(handle == nullptr);

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("CommDispatcher: Create with non-existent extension fails gracefully", "[comm_dispatcher]")
{
    ADUC_CommProviderConfig provCfg = {};
    provCfg.extensionId = "non_existent_extension_xyz";
    provCfg.priority = 1;
    provCfg.reportOnly = false;

    ADUC_CommDispatcherConfig config = {};
    config.providers = &provCfg;
    config.providerCount = 1;
    config.failoverTimeoutMs = 1000;

    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    ADUC_CommDispatcherHandle handle = nullptr;
    result = ADUC_CommDispatcher_Create(&config, registry, &handle);
    // Should fail because extension doesn't exist in the registry
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    ADUC_CommDispatcher_Destroy(handle);
    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("CommDispatcher: NULL handle safety", "[comm_dispatcher]")
{
    // Create with NULL outHandle
    ADUC_CommDispatcherConfig config = {};
    config.providers = nullptr;
    config.providerCount = 0;

    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    result = ADUC_CommDispatcher_Create(&config, registry, nullptr);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    // Create with NULL config
    ADUC_CommDispatcherHandle handle = nullptr;
    result = ADUC_CommDispatcher_Create(nullptr, registry, &handle);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    // ConnectAll with NULL handle
    result = ADUC_CommDispatcher_ConnectAll(nullptr, nullptr);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    // Poll with NULL handle
    ADUC_CommMessage msg = {};
    result = ADUC_CommDispatcher_Poll(nullptr, &msg, 1000);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    // ReportState with NULL handle
    result = ADUC_CommDispatcher_ReportState(nullptr, nullptr);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    // HealthCheck with NULL handle
    ADUC_CommHealthStatus status = {};
    result = ADUC_CommDispatcher_HealthCheck(nullptr, &status);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("CommDispatcher: Create with no providers rejects config", "[comm_dispatcher]")
{
    ADUC_CommDispatcherConfig config = {};
    config.providers = nullptr;
    config.providerCount = 0;
    config.failoverTimeoutMs = 5000;

    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    ADUC_CommDispatcherHandle handle = nullptr;
    result = ADUC_CommDispatcher_Create(&config, registry, &handle);
    // Create correctly rejects config with no providers
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
    CHECK(handle == nullptr);

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("CommDispatcher: Destroy NULL handle does not crash", "[comm_dispatcher]")
{
    ADUC_CommDispatcher_Destroy(nullptr); // Should not crash
}
