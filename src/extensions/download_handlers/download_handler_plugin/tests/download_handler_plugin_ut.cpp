/**
 * @file download_handler_plugin_ut.cpp
 * @brief Unit Tests for DownloadHandlerPlugin class and C API wrapper.
 *
 * Uses three test plugin shared libraries:
 *   - libtest_dh_plugin.so         — full plugin: success/failure via SetShouldFail(0/1)
 *   - libtest_dh_plugin_minimal.so — only Initialize; missing Cleanup/ProcessUpdate/etc.
 *   - libtest_dh_plugin_throwing.so — all exports present but throw exceptions via mode 1/2
 *
 * This exercises all code paths including every catch block in every method.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

#include "aduc/download_handler_plugin.h"
#include "aduc/download_handler_plugin.hpp"
#include <aduc/contract_utils.h>
#include <aduc/logging.h>
#include <aduc/result.h>
#include <aduc/shared_lib.hpp>
#include <aduc/types/update_content.h>

#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <string>

#ifndef TEST_PLUGIN_SO_PATH
#    error "TEST_PLUGIN_SO_PATH must be defined by CMakeLists.txt"
#endif

#ifndef TEST_MINIMAL_PLUGIN_SO_PATH
#    error "TEST_MINIMAL_PLUGIN_SO_PATH must be defined by CMakeLists.txt"
#endif

#ifndef TEST_THROWING_PLUGIN_SO_PATH
#    error "TEST_THROWING_PLUGIN_SO_PATH must be defined by CMakeLists.txt"
#endif

// =====================================================================
// Helpers
// =====================================================================

/// Toggle the test plugin's failure mode via dlopen/dlsym.
/// Must be called AFTER the DownloadHandlerPlugin constructor has loaded the lib.
/// Uses RTLD_NOLOAD to get the existing handle without adding a new reference.
static void SetPluginShouldFail(const char* soPath, int mode)
{
    void* handle = dlopen(soPath, RTLD_LAZY | RTLD_NOLOAD);
    REQUIRE(handle != nullptr);
    auto fn = reinterpret_cast<void (*)(int)>(dlsym(handle, "SetShouldFail"));
    REQUIRE(fn != nullptr);
    fn(mode);
    dlclose(handle);
}

// =====================================================================
// C API: ADUC_DownloadHandlerPlugin_OnUpdateWorkflowCompleted
// =====================================================================

TEST_CASE("OnUpdateWorkflowCompleted C API returns failure when handle is nullptr")
{
    ADUC_Result result = ADUC_DownloadHandlerPlugin_OnUpdateWorkflowCompleted(nullptr, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode != 0);
}

TEST_CASE("OnUpdateWorkflowCompleted C API returns failure when handle is nullptr but workflow non-null")
{
    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);

    ADUC_Result result = ADUC_DownloadHandlerPlugin_OnUpdateWorkflowCompleted(nullptr, wfHandle);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode != 0);
}

// =====================================================================
// DownloadHandlerPlugin class tests — full plugin (success/fail)
// =====================================================================

TEST_CASE("DownloadHandlerPlugin constructor succeeds with test plugin")
{
    std::string pluginPath(TEST_PLUGIN_SO_PATH);
    REQUIRE_NOTHROW([&]() {
        DownloadHandlerPlugin plugin(pluginPath, ADUC_LOG_DEBUG);
    }());
}

TEST_CASE("DownloadHandlerPlugin constructor throws with non-existent library")
{
    REQUIRE_THROWS_AS(
        DownloadHandlerPlugin("/nonexistent/path/to/plugin.so", ADUC_LOG_DEBUG),
        std::runtime_error);
}

TEST_CASE("DownloadHandlerPlugin ProcessUpdate returns success from test plugin")
{
    std::string pluginPath(TEST_PLUGIN_SO_PATH);
    DownloadHandlerPlugin plugin(pluginPath, ADUC_LOG_DEBUG);

    SetPluginShouldFail(TEST_PLUGIN_SO_PATH, 0);

    ADUC_FileEntity entity{};
    int dummyWf = 0;
    ADUC_WorkflowHandle wfHandle = &dummyWf;

    ADUC_Result result = plugin.ProcessUpdate(wfHandle, &entity, "/tmp/target");

    CHECK(result.ResultCode == ADUC_Result_Download_Handler_SuccessSkipDownload);
    CHECK(result.ExtendedResultCode == 0);
}

TEST_CASE("DownloadHandlerPlugin ProcessUpdate returns failure from test plugin")
{
    std::string pluginPath(TEST_PLUGIN_SO_PATH);
    DownloadHandlerPlugin plugin(pluginPath, ADUC_LOG_DEBUG);

    SetPluginShouldFail(TEST_PLUGIN_SO_PATH, 1);

    ADUC_FileEntity entity{};
    int dummyWf = 0;
    ADUC_WorkflowHandle wfHandle = &dummyWf;

    ADUC_Result result = plugin.ProcessUpdate(wfHandle, &entity, "/tmp/target");

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == 0x12345678);

    SetPluginShouldFail(TEST_PLUGIN_SO_PATH, 0);
}

TEST_CASE("DownloadHandlerPlugin OnUpdateWorkflowCompleted success")
{
    std::string pluginPath(TEST_PLUGIN_SO_PATH);
    DownloadHandlerPlugin plugin(pluginPath, ADUC_LOG_DEBUG);

    SetPluginShouldFail(TEST_PLUGIN_SO_PATH, 0);

    int dummyWf = 0;
    ADUC_WorkflowHandle wfHandle = &dummyWf;

    ADUC_Result result = plugin.OnUpdateWorkflowCompleted(wfHandle);

    CHECK(result.ResultCode == ADUC_GeneralResult_Success);
    CHECK(result.ExtendedResultCode == 0);
}

TEST_CASE("DownloadHandlerPlugin OnUpdateWorkflowCompleted failure")
{
    std::string pluginPath(TEST_PLUGIN_SO_PATH);
    DownloadHandlerPlugin plugin(pluginPath, ADUC_LOG_DEBUG);

    SetPluginShouldFail(TEST_PLUGIN_SO_PATH, 1);

    int dummyWf = 0;
    ADUC_WorkflowHandle wfHandle = &dummyWf;

    ADUC_Result result = plugin.OnUpdateWorkflowCompleted(wfHandle);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == static_cast<int>(0x87654321));

    SetPluginShouldFail(TEST_PLUGIN_SO_PATH, 0);
}

TEST_CASE("DownloadHandlerPlugin GetContractInfo returns v1.0")
{
    std::string pluginPath(TEST_PLUGIN_SO_PATH);

    DownloadHandlerPlugin plugin(pluginPath, ADUC_LOG_DEBUG);

    ADUC_ExtensionContractInfo ci{};
    ci.majorVer = 99;
    ci.minorVer = 99;

    ADUC_Result result = plugin.GetContractInfo(&ci);

    CHECK(result.ResultCode == ADUC_GeneralResult_Success);
    CHECK(ci.majorVer == ADUC_V1_CONTRACT_MAJOR_VER);
    CHECK(ci.minorVer == ADUC_V1_CONTRACT_MINOR_VER);
}

TEST_CASE("C API OnUpdateWorkflowCompleted with valid handle delegates to plugin")
{
    std::string pluginPath(TEST_PLUGIN_SO_PATH);
    auto* plugin = new DownloadHandlerPlugin(pluginPath, ADUC_LOG_DEBUG);

    SetPluginShouldFail(TEST_PLUGIN_SO_PATH, 0);
    DownloadHandlerHandle handle = reinterpret_cast<DownloadHandlerHandle>(plugin);

    int dummyWf = 0;
    ADUC_WorkflowHandle wfHandle = &dummyWf;

    ADUC_Result result = ADUC_DownloadHandlerPlugin_OnUpdateWorkflowCompleted(handle, wfHandle);

    CHECK(result.ResultCode == ADUC_GeneralResult_Success);

    delete plugin;
}

TEST_CASE("C API OnUpdateWorkflowCompleted returns plugin failure when plugin reports failure")
{
    std::string pluginPath(TEST_PLUGIN_SO_PATH);
    auto* plugin = new DownloadHandlerPlugin(pluginPath, ADUC_LOG_DEBUG);

    SetPluginShouldFail(TEST_PLUGIN_SO_PATH, 1);
    DownloadHandlerHandle handle = reinterpret_cast<DownloadHandlerHandle>(plugin);

    int dummyWf = 0;
    ADUC_WorkflowHandle wfHandle = &dummyWf;

    ADUC_Result result = ADUC_DownloadHandlerPlugin_OnUpdateWorkflowCompleted(handle, wfHandle);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == static_cast<int>(0x87654321));

    SetPluginShouldFail(TEST_PLUGIN_SO_PATH, 0);
    delete plugin;
}

// =====================================================================
// Minimal plugin tests — PluginException catch blocks
// =====================================================================

TEST_CASE("Destructor handles missing Cleanup gracefully (PluginException)")
{
    // Minimal plugin has Initialize but no Cleanup. Destructor catches PluginException.
    {
        DownloadHandlerPlugin plugin(TEST_MINIMAL_PLUGIN_SO_PATH, ADUC_LOG_DEBUG);
        // plugin goes out of scope
    }
    SUCCEED("Destructor did not crash despite missing Cleanup");
}

TEST_CASE("ProcessUpdate catches PluginException for missing export")
{
    DownloadHandlerPlugin plugin(TEST_MINIMAL_PLUGIN_SO_PATH, ADUC_LOG_DEBUG);

    ADUC_FileEntity entity{};
    int dummyWf = 0;

    ADUC_Result result = plugin.ProcessUpdate(&dummyWf, &entity, "/tmp/target");

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_PLUGIN_EXPORT_CALL_PROCESSUPDATE);
}

TEST_CASE("OnUpdateWorkflowCompleted catches PluginException for missing export")
{
    DownloadHandlerPlugin plugin(TEST_MINIMAL_PLUGIN_SO_PATH, ADUC_LOG_DEBUG);

    int dummyWf = 0;

    ADUC_Result result = plugin.OnUpdateWorkflowCompleted(&dummyWf);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_PLUGIN_EXPORT_CALL_ONUPDATEWORKFLOWCOMPLETED);
}

TEST_CASE("GetContractInfo catches PluginException for missing export")
{
    DownloadHandlerPlugin plugin(TEST_MINIMAL_PLUGIN_SO_PATH, ADUC_LOG_DEBUG);

    ADUC_ExtensionContractInfo ci{};

    ADUC_Result result = plugin.GetContractInfo(&ci);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_PLUGIN_EXPORT_CALL_GETCONTRACTINFO);
}

// =====================================================================
// Throwing plugin tests — std::exception and non-std exception paths
// =====================================================================

/// Toggle the throwing plugin's failure mode via dlopen/dlsym.
static void SetThrowingPluginMode(const char* soPath, int mode)
{
    void* handle = dlopen(soPath, RTLD_LAZY | RTLD_NOLOAD);
    REQUIRE(handle != nullptr);
    auto fn = reinterpret_cast<void (*)(int)>(dlsym(handle, "SetShouldFail"));
    REQUIRE(fn != nullptr);
    fn(mode);
    dlclose(handle);
}

TEST_CASE("ProcessUpdate catches std::exception from plugin")
{
    DownloadHandlerPlugin plugin(TEST_THROWING_PLUGIN_SO_PATH, ADUC_LOG_DEBUG);
    SetThrowingPluginMode(TEST_THROWING_PLUGIN_SO_PATH, 1);

    ADUC_FileEntity entity{};
    int dummyWf = 0;

    ADUC_Result result = plugin.ProcessUpdate(&dummyWf, &entity, "/tmp/target");

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);

    SetThrowingPluginMode(TEST_THROWING_PLUGIN_SO_PATH, 0);
}

TEST_CASE("ProcessUpdate catches non-std exception from plugin")
{
    DownloadHandlerPlugin plugin(TEST_THROWING_PLUGIN_SO_PATH, ADUC_LOG_DEBUG);
    SetThrowingPluginMode(TEST_THROWING_PLUGIN_SO_PATH, 2);

    ADUC_FileEntity entity{};
    int dummyWf = 0;

    ADUC_Result result = plugin.ProcessUpdate(&dummyWf, &entity, "/tmp/target");

    CHECK(result.ResultCode == ADUC_Result_Failure);

    SetThrowingPluginMode(TEST_THROWING_PLUGIN_SO_PATH, 0);
}

TEST_CASE("OnUpdateWorkflowCompleted catches std::exception from plugin")
{
    DownloadHandlerPlugin plugin(TEST_THROWING_PLUGIN_SO_PATH, ADUC_LOG_DEBUG);
    SetThrowingPluginMode(TEST_THROWING_PLUGIN_SO_PATH, 1);

    int dummyWf = 0;

    ADUC_Result result = plugin.OnUpdateWorkflowCompleted(&dummyWf);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);

    SetThrowingPluginMode(TEST_THROWING_PLUGIN_SO_PATH, 0);
}

TEST_CASE("OnUpdateWorkflowCompleted catches non-std exception from plugin")
{
    DownloadHandlerPlugin plugin(TEST_THROWING_PLUGIN_SO_PATH, ADUC_LOG_DEBUG);
    SetThrowingPluginMode(TEST_THROWING_PLUGIN_SO_PATH, 2);

    int dummyWf = 0;

    ADUC_Result result = plugin.OnUpdateWorkflowCompleted(&dummyWf);

    CHECK(result.ResultCode == ADUC_Result_Failure);

    SetThrowingPluginMode(TEST_THROWING_PLUGIN_SO_PATH, 0);
}

TEST_CASE("GetContractInfo catches std::exception from plugin")
{
    DownloadHandlerPlugin plugin(TEST_THROWING_PLUGIN_SO_PATH, ADUC_LOG_DEBUG);
    SetThrowingPluginMode(TEST_THROWING_PLUGIN_SO_PATH, 1);

    ADUC_ExtensionContractInfo ci{};

    ADUC_Result result = plugin.GetContractInfo(&ci);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);

    SetThrowingPluginMode(TEST_THROWING_PLUGIN_SO_PATH, 0);
}

TEST_CASE("GetContractInfo catches non-std exception from plugin")
{
    DownloadHandlerPlugin plugin(TEST_THROWING_PLUGIN_SO_PATH, ADUC_LOG_DEBUG);
    SetThrowingPluginMode(TEST_THROWING_PLUGIN_SO_PATH, 2);

    ADUC_ExtensionContractInfo ci{};

    ADUC_Result result = plugin.GetContractInfo(&ci);

    CHECK(result.ResultCode == ADUC_Result_Failure);

    SetThrowingPluginMode(TEST_THROWING_PLUGIN_SO_PATH, 0);
}

TEST_CASE("Destructor catches std::exception from Cleanup")
{
    {
        DownloadHandlerPlugin plugin(TEST_THROWING_PLUGIN_SO_PATH, ADUC_LOG_DEBUG);
        SetThrowingPluginMode(TEST_THROWING_PLUGIN_SO_PATH, 3);
        // plugin goes out of scope — destructor calls Cleanup which throws std::exception
    }
    SUCCEED("Destructor handled std::exception gracefully");
}

TEST_CASE("Destructor catches non-std exception from Cleanup")
{
    {
        DownloadHandlerPlugin plugin(TEST_THROWING_PLUGIN_SO_PATH, ADUC_LOG_DEBUG);
        SetThrowingPluginMode(TEST_THROWING_PLUGIN_SO_PATH, 4);
        // plugin goes out of scope — destructor calls Cleanup which throws int
    }
    SUCCEED("Destructor handled non-std exception gracefully");
}
