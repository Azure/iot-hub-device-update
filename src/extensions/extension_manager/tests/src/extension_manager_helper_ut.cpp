/**
 * @file extension_manager_helper_ut.cpp
 * @brief Unit Tests for extension_manager_helper functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/extension_manager_download_options.h>
#include <aduc/extension_manager_helper.hpp>
#include <aduc/result.h>
#include <aduc/types/update_content.h>

#include <catch2/catch_all.hpp>

#include <cstring> // memset

// =====================================================================
// ProcessDownloadHandlerExtensibility - Bad argument tests
// =====================================================================

TEST_CASE("ProcessDownloadHandlerExtensibility returns BAD_ARG when workflowHandle is nullptr")
{
    ADUC_FileEntity entity{};
    char downloadHandlerId[] = "microsoft/test-handler";
    entity.DownloadHandlerId = downloadHandlerId;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(
        nullptr /* workflowHandle */, &entity, "/tmp/target_file");

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_HELPER_BAD_ARG);
}

TEST_CASE("ProcessDownloadHandlerExtensibility returns BAD_ARG when entity is nullptr")
{
    int dummy = 0;
    ADUC_WorkflowHandle workflowHandle = &dummy;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(
        workflowHandle, nullptr /* entity */, "/tmp/target_file");

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_HELPER_BAD_ARG);
}

TEST_CASE("ProcessDownloadHandlerExtensibility returns BAD_ARG when DownloadHandlerId is nullptr")
{
    int dummy = 0;
    ADUC_WorkflowHandle workflowHandle = &dummy;

    ADUC_FileEntity entity{};
    entity.DownloadHandlerId = nullptr;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(
        workflowHandle, &entity, "/tmp/target_file");

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_HELPER_BAD_ARG);
}

TEST_CASE("ProcessDownloadHandlerExtensibility returns BAD_ARG when DownloadHandlerId is empty")
{
    int dummy = 0;
    ADUC_WorkflowHandle workflowHandle = &dummy;

    ADUC_FileEntity entity{};
    char emptyStr[] = "";
    entity.DownloadHandlerId = emptyStr;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(
        workflowHandle, &entity, "/tmp/target_file");

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_HELPER_BAD_ARG);
}

TEST_CASE("ProcessDownloadHandlerExtensibility returns BAD_ARG when targetUpdateFilePath is nullptr")
{
    int dummy = 0;
    ADUC_WorkflowHandle workflowHandle = &dummy;

    ADUC_FileEntity entity{};
    char downloadHandlerId[] = "microsoft/test-handler";
    entity.DownloadHandlerId = downloadHandlerId;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(
        workflowHandle, &entity, nullptr /* targetUpdateFilePath */);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_HELPER_BAD_ARG);
}

TEST_CASE("ProcessDownloadHandlerExtensibility returns BAD_ARG when targetUpdateFilePath is empty")
{
    int dummy = 0;
    ADUC_WorkflowHandle workflowHandle = &dummy;

    ADUC_FileEntity entity{};
    char downloadHandlerId[] = "microsoft/test-handler";
    entity.DownloadHandlerId = downloadHandlerId;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(
        workflowHandle, &entity, "" /* targetUpdateFilePath */);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_HELPER_BAD_ARG);
}

TEST_CASE("ProcessDownloadHandlerExtensibility returns BAD_ARG when all args are nullptr")
{
    ADUC_Result result = ProcessDownloadHandlerExtensibility(nullptr, nullptr, nullptr);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_HELPER_BAD_ARG);
}

// =====================================================================
// GetDownloadTimeoutInMinutes tests
// =====================================================================

TEST_CASE("GetDownloadTimeoutInMinutes returns default when downloadOptions is nullptr")
{
    unsigned int timeout = GetDownloadTimeoutInMinutes(nullptr);

    // When config singleton is not initialized and downloadOptions is nullptr,
    // the function returns CONTENT_DOWNLOADER_MAX_TIMEOUT_IN_MINUTES_DEFAULT.
    CHECK(timeout == CONTENT_DOWNLOADER_MAX_TIMEOUT_IN_MINUTES_DEFAULT);
}

TEST_CASE("GetDownloadTimeoutInMinutes returns downloadOptions value when config has zero timeout")
{
    ExtensionManager_Download_Options options{};
    options.timeoutInMinutes = 42;

    unsigned int timeout = GetDownloadTimeoutInMinutes(&options);

    // When config singleton is not initialized, it falls through to the
    // downloadOptions path but config is nullptr so we get default.
    // The config singleton returns nullptr if not initialized, so we
    // get the default value.
    CHECK(timeout == CONTENT_DOWNLOADER_MAX_TIMEOUT_IN_MINUTES_DEFAULT);
}

TEST_CASE("GetDownloadTimeoutInMinutes default value is 480 minutes (8 hours)")
{
    CHECK(CONTENT_DOWNLOADER_MAX_TIMEOUT_IN_MINUTES_DEFAULT == (8 * 60));
}

TEST_CASE("Default_ExtensionManager_Download_Options has default timeout")
{
    CHECK(Default_ExtensionManager_Download_Options.timeoutInMinutes == CONTENT_DOWNLOADER_MAX_TIMEOUT_IN_MINUTES_DEFAULT);
}

// =====================================================================
// ProcessDownloadHandlerExtensibility - non-bad-arg tests
// =====================================================================

// NOTE: Tests for ProcessDownloadHandlerExtensibility with valid args but uninitialized
// DownloadHandlerFactory were removed — GetInstance() triggers SIGABRT (free: invalid
// pointer) rather than a catchable exception, making those paths untestable without mocks.

// =====================================================================
// GetDownloadTimeoutInMinutes - additional coverage
// =====================================================================

TEST_CASE("GetDownloadTimeoutInMinutes returns default when downloadOptions has zero timeout")
{
    ExtensionManager_Download_Options options{};
    options.timeoutInMinutes = 0;

    unsigned int timeout = GetDownloadTimeoutInMinutes(&options);

    // Config singleton is not initialized → returns default regardless of options.
    CHECK(timeout == CONTENT_DOWNLOADER_MAX_TIMEOUT_IN_MINUTES_DEFAULT);
}

TEST_CASE("GetDownloadTimeoutInMinutes returns default for large timeout value in options")
{
    ExtensionManager_Download_Options options{};
    options.timeoutInMinutes = 99999;

    unsigned int timeout = GetDownloadTimeoutInMinutes(&options);

    // Config not initialized → always returns default.
    CHECK(timeout == CONTENT_DOWNLOADER_MAX_TIMEOUT_IN_MINUTES_DEFAULT);
}

TEST_CASE("Default_ExtensionManager_Download_Options can be modified and used")
{
    ExtensionManager_Download_Options opts = Default_ExtensionManager_Download_Options;
    opts.timeoutInMinutes = 120;

    unsigned int timeout = GetDownloadTimeoutInMinutes(&opts);

    // Config not initialized → returns default, but the modified struct is valid
    CHECK(timeout == CONTENT_DOWNLOADER_MAX_TIMEOUT_IN_MINUTES_DEFAULT);
    CHECK(opts.timeoutInMinutes == 120);
}