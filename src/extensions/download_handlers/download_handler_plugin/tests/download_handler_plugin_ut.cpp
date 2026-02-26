/**
 * @file download_handler_plugin_ut.cpp
 * @brief Unit Tests for DownloadHandlerPlugin class and C API wrapper.
 *
 * Uses a test plugin shared library (libtest_dh_plugin.so) to exercise
 * the full code paths of DownloadHandlerPlugin constructor, destructor,
 * ProcessUpdate, OnUpdateWorkflowCompleted, GetContractInfo, and the
 * ADUC_DownloadHandlerPlugin_OnUpdateWorkflowCompleted C API.
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

// =====================================================================
// Helpers
// =====================================================================

/// Toggle the test plugin's failure mode via dlopen/dlsym.
/// Must be called AFTER the DownloadHandlerPlugin constructor has loaded the lib.
/// Uses RTLD_NOLOAD to get the existing handle without adding a new reference.
static void SetPluginShouldFail(int fail)
{
    void* handle = dlopen(TEST_PLUGIN_SO_PATH, RTLD_LAZY | RTLD_NOLOAD);
    REQUIRE(handle != nullptr);
    auto fn = reinterpret_cast<void (*)(int)>(dlsym(handle, "SetShouldFail"));
    REQUIRE(fn != nullptr);
    fn(fail);
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
// DownloadHandlerPlugin class tests (using test plugin .so)
// =====================================================================

TEST_CASE("DownloadHandlerPlugin constructor succeeds with test plugin")
{
    std::string pluginPath(TEST_PLUGIN_SO_PATH);

    // Constructor calls Initialize on the plugin. Should not throw.
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

    // Set after construction so we modify the already-loaded library instance.
    SetPluginShouldFail(0);

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

    // Set failure mode after construction so we modify the already-loaded library.
    SetPluginShouldFail(1);

    ADUC_FileEntity entity{};
    int dummyWf = 0;
    ADUC_WorkflowHandle wfHandle = &dummyWf;

    ADUC_Result result = plugin.ProcessUpdate(wfHandle, &entity, "/tmp/target");

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == 0x12345678);

    SetPluginShouldFail(0);
}

TEST_CASE("DownloadHandlerPlugin OnUpdateWorkflowCompleted success")
{
    std::string pluginPath(TEST_PLUGIN_SO_PATH);
    DownloadHandlerPlugin plugin(pluginPath, ADUC_LOG_DEBUG);

    SetPluginShouldFail(0);

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

    // Set failure mode after construction.
    SetPluginShouldFail(1);

    int dummyWf = 0;
    ADUC_WorkflowHandle wfHandle = &dummyWf;

    ADUC_Result result = plugin.OnUpdateWorkflowCompleted(wfHandle);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == static_cast<int>(0x87654321));

    SetPluginShouldFail(0);
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

    SetPluginShouldFail(0);
    DownloadHandlerHandle handle = reinterpret_cast<DownloadHandlerHandle>(plugin);

    int dummyWf = 0;
    ADUC_WorkflowHandle wfHandle = &dummyWf;

    ADUC_Result result = ADUC_DownloadHandlerPlugin_OnUpdateWorkflowCompleted(handle, wfHandle);

    CHECK(result.ResultCode == ADUC_GeneralResult_Success);

    delete plugin;
}
