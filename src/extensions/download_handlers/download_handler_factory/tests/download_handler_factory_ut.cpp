/**
 * @file download_handler_factory_ut.cpp
 * @brief Unit Tests for DownloadHandlerFactory.
 *
 * Uses link-time mocks for GetDownloadHandlerFileEntity,
 * ADUC_HashUtils_VerifyWithStrongestHash, and ADUC_FileEntity_Uninit
 * to control the factory's behavior without real extensions on disk.
 *
 * Two test plugin shared libraries are built by CMakeLists.txt:
 *   - libtest_factory_plugin.so       — has Initialize + Cleanup
 *   - libtest_factory_noinit_plugin.so — missing Initialize (triggers PluginException)
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

extern "C" {
#include "aduc/download_handler_factory.h"
}
#include "aduc/download_handler_factory.hpp"
#include "mock_factory_deps.h"

#include <string>

#ifndef TEST_FACTORY_PLUGIN_PATH
#    error "TEST_FACTORY_PLUGIN_PATH must be defined by CMakeLists.txt"
#endif

#ifndef TEST_FACTORY_NOINIT_PLUGIN_PATH
#    error "TEST_FACTORY_NOINIT_PLUGIN_PATH must be defined by CMakeLists.txt"
#endif

// =====================================================================
// DownloadHandlerFactory singleton tests
// =====================================================================

TEST_CASE("DownloadHandlerFactory GetInstance returns non-null")
{
    DownloadHandlerFactory* factory = DownloadHandlerFactory::GetInstance();
    CHECK(factory != nullptr);
}

TEST_CASE("DownloadHandlerFactory GetInstance returns same pointer on repeated calls")
{
    DownloadHandlerFactory* factory1 = DownloadHandlerFactory::GetInstance();
    DownloadHandlerFactory* factory2 = DownloadHandlerFactory::GetInstance();
    CHECK(factory1 == factory2);
}

TEST_CASE("DownloadHandlerFactory GetInstance is stable across many calls")
{
    DownloadHandlerFactory* first = DownloadHandlerFactory::GetInstance();
    for (int i = 0; i < 100; ++i)
    {
        CHECK(DownloadHandlerFactory::GetInstance() == first);
    }
}

// =====================================================================
// LoadDownloadHandler tests
// =====================================================================

TEST_CASE("LoadDownloadHandler returns nullptr when GetDownloadHandlerFileEntity fails")
{
    g_mockFactoryDeps.getFileEntityResult = false;
    g_mockFactoryDeps.fileEntityPath = nullptr;
    g_mockFactoryDeps.verifyHashResult = false;

    DownloadHandlerFactory* factory = DownloadHandlerFactory::GetInstance();
    DownloadHandlerPlugin* result = factory->LoadDownloadHandler("fail_entity_id");

    CHECK(result == nullptr);
}

TEST_CASE("LoadDownloadHandler returns nullptr when hash verification fails")
{
    g_mockFactoryDeps.getFileEntityResult = true;
    g_mockFactoryDeps.fileEntityPath = TEST_FACTORY_PLUGIN_PATH;
    g_mockFactoryDeps.verifyHashResult = false;

    DownloadHandlerFactory* factory = DownloadHandlerFactory::GetInstance();
    DownloadHandlerPlugin* result = factory->LoadDownloadHandler("fail_hash_id");

    CHECK(result == nullptr);
}

TEST_CASE("LoadDownloadHandler succeeds with valid test plugin")
{
    g_mockFactoryDeps.getFileEntityResult = true;
    g_mockFactoryDeps.fileEntityPath = TEST_FACTORY_PLUGIN_PATH;
    g_mockFactoryDeps.verifyHashResult = true;

    DownloadHandlerFactory* factory = DownloadHandlerFactory::GetInstance();
    DownloadHandlerPlugin* result = factory->LoadDownloadHandler("success_id");

    CHECK(result != nullptr);
}

TEST_CASE("LoadDownloadHandler returns cached plugin on second call with same id")
{
    g_mockFactoryDeps.getFileEntityResult = true;
    g_mockFactoryDeps.fileEntityPath = TEST_FACTORY_PLUGIN_PATH;
    g_mockFactoryDeps.verifyHashResult = true;

    DownloadHandlerFactory* factory = DownloadHandlerFactory::GetInstance();

    DownloadHandlerPlugin* first = factory->LoadDownloadHandler("cache_test_id");
    REQUIRE(first != nullptr);

    // Second call with same id should return cached pointer
    DownloadHandlerPlugin* second = factory->LoadDownloadHandler("cache_test_id");
    CHECK(second == first);
}

TEST_CASE("LoadDownloadHandler returns nullptr on PluginException (missing Initialize)")
{
    g_mockFactoryDeps.getFileEntityResult = true;
    g_mockFactoryDeps.fileEntityPath = TEST_FACTORY_NOINIT_PLUGIN_PATH;
    g_mockFactoryDeps.verifyHashResult = true;

    DownloadHandlerFactory* factory = DownloadHandlerFactory::GetInstance();
    DownloadHandlerPlugin* result = factory->LoadDownloadHandler("plugin_exception_id");

    CHECK(result == nullptr);
}

TEST_CASE("LoadDownloadHandler returns nullptr on std::exception (bad path)")
{
    g_mockFactoryDeps.getFileEntityResult = true;
    g_mockFactoryDeps.fileEntityPath = "/nonexistent/path/to/plugin.so";
    g_mockFactoryDeps.verifyHashResult = true;

    DownloadHandlerFactory* factory = DownloadHandlerFactory::GetInstance();
    DownloadHandlerPlugin* result = factory->LoadDownloadHandler("bad_path_id");

    CHECK(result == nullptr);
}

// =====================================================================
// C API tests
// =====================================================================

TEST_CASE("C API LoadDownloadHandler returns valid handle on success")
{
    g_mockFactoryDeps.getFileEntityResult = true;
    g_mockFactoryDeps.fileEntityPath = TEST_FACTORY_PLUGIN_PATH;
    g_mockFactoryDeps.verifyHashResult = true;

    DownloadHandlerHandle handle = ADUC_DownloadHandlerFactory_LoadDownloadHandler("capi_success_id");

    CHECK(handle != nullptr);
}

TEST_CASE("C API LoadDownloadHandler returns nullptr on failure")
{
    g_mockFactoryDeps.getFileEntityResult = false;
    g_mockFactoryDeps.fileEntityPath = nullptr;
    g_mockFactoryDeps.verifyHashResult = false;

    DownloadHandlerHandle handle = ADUC_DownloadHandlerFactory_LoadDownloadHandler("capi_fail_id");

    CHECK(handle == nullptr);
}
