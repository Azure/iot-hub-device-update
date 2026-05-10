/**
 * @file extension_loader_ut.cpp
 * @brief Unit tests for the extension loader and registry.
 */

#include <catch2/catch_all.hpp>

extern "C"
{
#include "aduc/extension_loader.h"
#include "aduc/extension_types.h"
}

TEST_CASE("ExtensionRegistry: Create and verify empty", "[extension_loader]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);

    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    REQUIRE(registry != nullptr);

    CHECK(ADUC_ExtensionRegistry_GetCount(registry) == 0);

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("ExtensionRegistry: Create with NULL outHandle fails", "[extension_loader]")
{
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(nullptr);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

TEST_CASE("ExtensionRegistry: LoadFromFile with non-existent .so fails", "[extension_loader]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    result = ADUC_ExtensionRegistry_LoadFromFile(registry, "/no/such/library.so", nullptr);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    // Registry should still be empty after failed load
    CHECK(ADUC_ExtensionRegistry_GetCount(registry) == 0);

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("ExtensionRegistry: LoadFromFile with NULL path fails", "[extension_loader]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    result = ADUC_ExtensionRegistry_LoadFromFile(registry, nullptr, nullptr);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("ExtensionRegistry: FindById with no extensions returns NULL", "[extension_loader]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    CHECK(ADUC_ExtensionRegistry_FindById(registry, "nonexistent-id") == nullptr);

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("ExtensionRegistry: FindByType with no extensions returns NULL", "[extension_loader]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    CHECK(ADUC_ExtensionRegistry_FindByType(registry, ADUC_EXT_TYPE_STEP_HANDLER, 0) == nullptr);
    CHECK(ADUC_ExtensionRegistry_FindByType(registry, ADUC_EXT_TYPE_COMMUNICATION, 0) == nullptr);

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("ExtensionRegistry: FindByCapability with no extensions returns NULL", "[extension_loader]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    CHECK(ADUC_ExtensionRegistry_FindByCapability(registry, "microsoft/swupdate:2") == nullptr);

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("ExtensionRegistry: GetCount on empty registry is 0", "[extension_loader]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    CHECK(ADUC_ExtensionRegistry_GetCount(registry) == 0);

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("ExtensionRegistry: GetCountByType on empty registry is 0", "[extension_loader]")
{
    ADUC_ExtensionRegistryHandle registry = nullptr;
    ADUC_Result2 result = ADUC_ExtensionRegistry_Create(&registry);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    CHECK(ADUC_ExtensionRegistry_GetCountByType(registry, ADUC_EXT_TYPE_STEP_HANDLER) == 0);
    CHECK(ADUC_ExtensionRegistry_GetCountByType(registry, ADUC_EXT_TYPE_DOWNLOADER) == 0);
    CHECK(ADUC_ExtensionRegistry_GetCountByType(registry, ADUC_EXT_TYPE_AUTHENTICATOR) == 0);

    ADUC_ExtensionRegistry_Destroy(registry);
}

TEST_CASE("ExtensionRegistry: Destroy NULL is safe", "[extension_loader]")
{
    ADUC_ExtensionRegistry_Destroy(nullptr); // Should not crash
}

TEST_CASE("ExtensionRegistry: GetCount on NULL registry returns 0", "[extension_loader]")
{
    CHECK(ADUC_ExtensionRegistry_GetCount(nullptr) == 0);
    CHECK(ADUC_ExtensionRegistry_GetCountByType(nullptr, ADUC_EXT_TYPE_STEP_HANDLER) == 0);
}

TEST_CASE("ExtensionRegistry: FindById on NULL registry returns NULL", "[extension_loader]")
{
    CHECK(ADUC_ExtensionRegistry_FindById(nullptr, "some-id") == nullptr);
}

TEST_CASE("ExtensionRegistry: FindByCapability on NULL registry returns NULL", "[extension_loader]")
{
    CHECK(ADUC_ExtensionRegistry_FindByCapability(nullptr, "cap") == nullptr);
}
