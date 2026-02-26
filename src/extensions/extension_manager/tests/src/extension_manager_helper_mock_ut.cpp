/**
 * @file extension_manager_helper_mock_ut.cpp
 * @brief Mock-based Unit Tests for extension_manager_helper.cpp to maximize coverage.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "mock_extension_manager_helper_deps.h"

#include <aduc/contract_utils.h>
#include <aduc/extension_manager_download_options.h>
#include <aduc/extension_manager_helper.hpp>
#include <aduc/result.h>
#include <aduc/types/update_content.h>

#include <catch2/catch_all.hpp>
#include <cstring>

struct HelperFixture
{
    HelperFixture()
    {
        mock_extension_manager_helper_reset();
    }

    ~HelperFixture()
    {
        mock_extension_manager_helper_reset();
    }
};

// =====================================================================
// ProcessDownloadHandlerExtensibility tests  — bad arg paths
// =====================================================================

TEST_CASE_METHOD(HelperFixture, "Helper: returns BAD_ARG when workflowHandle is nullptr")
{
    ADUC_FileEntity entity{};
    char handlerId[] = "microsoft/test-handler";
    entity.DownloadHandlerId = handlerId;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(nullptr, &entity, "/tmp/target");
    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_HELPER_BAD_ARG);
}

TEST_CASE_METHOD(HelperFixture, "Helper: returns BAD_ARG when entity is nullptr")
{
    int dummy = 0;
    ADUC_Result result = ProcessDownloadHandlerExtensibility(&dummy, nullptr, "/tmp/target");
    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_HELPER_BAD_ARG);
}

TEST_CASE_METHOD(HelperFixture, "Helper: returns BAD_ARG when DownloadHandlerId is nullptr")
{
    int dummy = 0;
    ADUC_FileEntity entity{};
    entity.DownloadHandlerId = nullptr;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(&dummy, &entity, "/tmp/target");
    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_HELPER_BAD_ARG);
}

TEST_CASE_METHOD(HelperFixture, "Helper: returns BAD_ARG when DownloadHandlerId is empty")
{
    int dummy = 0;
    ADUC_FileEntity entity{};
    char emptyStr[] = "";
    entity.DownloadHandlerId = emptyStr;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(&dummy, &entity, "/tmp/target");
    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_HELPER_BAD_ARG);
}

TEST_CASE_METHOD(HelperFixture, "Helper: returns BAD_ARG when targetUpdateFilePath is nullptr")
{
    int dummy = 0;
    ADUC_FileEntity entity{};
    char handlerId[] = "microsoft/test-handler";
    entity.DownloadHandlerId = handlerId;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(&dummy, &entity, nullptr);
    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_HELPER_BAD_ARG);
}

TEST_CASE_METHOD(HelperFixture, "Helper: returns BAD_ARG when targetUpdateFilePath is empty")
{
    int dummy = 0;
    ADUC_FileEntity entity{};
    char handlerId[] = "microsoft/test-handler";
    entity.DownloadHandlerId = handlerId;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(&dummy, &entity, "");
    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_HELPER_BAD_ARG);
}

TEST_CASE_METHOD(HelperFixture, "Helper: returns BAD_ARG when all args are nullptr")
{
    ADUC_Result result = ProcessDownloadHandlerExtensibility(nullptr, nullptr, nullptr);
    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_HELPER_BAD_ARG);
}

// =====================================================================
// ProcessDownloadHandlerExtensibility — factory exception
// =====================================================================

TEST_CASE_METHOD(HelperFixture, "Helper: returns FACTORY_INSTANCE error when factory throws")
{
    mock_factory_get_instance_throws = true;

    int dummy = 0;
    ADUC_FileEntity entity{};
    char handlerId[] = "microsoft/test-handler";
    entity.DownloadHandlerId = handlerId;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(&dummy, &entity, "/tmp/target");
    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_CREATE_FACTORY_INSTANCE);
}

// =====================================================================
// ProcessDownloadHandlerExtensibility — load failure
// =====================================================================

TEST_CASE_METHOD(HelperFixture, "Helper: returns error when LoadDownloadHandler returns null")
{
    mock_factory_load_returns_null = true;

    int dummy = 0;
    ADUC_FileEntity entity{};
    char handlerId[] = "microsoft/test-handler";
    entity.DownloadHandlerId = handlerId;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(&dummy, &entity, "/tmp/target");
    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_CREATE_FAILURE_CREATE);
    CHECK(mock_helper_workflow_add_erc_count == 1);
}

// =====================================================================
// ProcessDownloadHandlerExtensibility — GetContractInfo failure
// =====================================================================

TEST_CASE_METHOD(HelperFixture, "Helper: returns error when GetContractInfo fails")
{
    mock_plugin_get_contract_info_result = { ADUC_GeneralResult_Failure, static_cast<ADUC_Result_t>(0x99887766) };

    int dummy = 0;
    ADUC_FileEntity entity{};
    char handlerId[] = "microsoft/test-handler";
    entity.DownloadHandlerId = handlerId;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(&dummy, &entity, "/tmp/target");
    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(mock_helper_workflow_add_erc_count == 1);
}

// =====================================================================
// ProcessDownloadHandlerExtensibility — unsupported contract version
// =====================================================================

TEST_CASE_METHOD(HelperFixture, "Helper: returns error when contract version is unsupported")
{
    // Plugin has contract 2.0 which is not v1
    mock_plugin_contract_info = { 2, 0 };
    mock_plugin_get_contract_info_result = { 1, 0 }; // GetContractInfo succeeds

    int dummy = 0;
    ADUC_FileEntity entity{};
    char handlerId[] = "microsoft/test-handler";
    entity.DownloadHandlerId = handlerId;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(&dummy, &entity, "/tmp/target");
    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_UNSUPPORTED_CONTRACT_VERSION);
    CHECK(mock_helper_workflow_add_erc_count == 1);
}

TEST_CASE_METHOD(HelperFixture, "Helper: returns error when contract version has wrong minor ver")
{
    // Plugin has contract 1.5 which is not exactly v1.0
    mock_plugin_contract_info = { 1, 5 };
    mock_plugin_get_contract_info_result = { 1, 0 };

    int dummy = 0;
    ADUC_FileEntity entity{};
    char handlerId[] = "microsoft/test-handler";
    entity.DownloadHandlerId = handlerId;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(&dummy, &entity, "/tmp/target");
    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_UNSUPPORTED_CONTRACT_VERSION);
}

// =====================================================================
// ProcessDownloadHandlerExtensibility — ProcessUpdate failure
// =====================================================================

TEST_CASE_METHOD(HelperFixture, "Helper: returns error when ProcessUpdate fails")
{
    // Contract is v1.0 — passes validation
    mock_plugin_contract_info = { ADUC_V1_CONTRACT_MAJOR_VER, ADUC_V1_CONTRACT_MINOR_VER };
    mock_plugin_get_contract_info_result = { 1, 0 };
    mock_plugin_process_update_result = { ADUC_GeneralResult_Failure, static_cast<ADUC_Result_t>(0xAABBCCDD) };

    int dummy = 0;
    ADUC_FileEntity entity{};
    char handlerId[] = "microsoft/test-handler";
    entity.DownloadHandlerId = handlerId;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(&dummy, &entity, "/tmp/target");
    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == static_cast<ADUC_Result_t>(0xAABBCCDD));
    CHECK(mock_helper_workflow_add_erc_count == 1);
    CHECK(mock_helper_workflow_set_result_details_count == 1);
}

// =====================================================================
// ProcessDownloadHandlerExtensibility — success path
// =====================================================================

TEST_CASE_METHOD(HelperFixture, "Helper: succeeds for valid download handler")
{
    mock_plugin_contract_info = { ADUC_V1_CONTRACT_MAJOR_VER, ADUC_V1_CONTRACT_MINOR_VER };
    mock_plugin_get_contract_info_result = { 1, 0 };
    mock_plugin_process_update_result = { 1, 0 };

    int dummy = 0;
    ADUC_FileEntity entity{};
    char handlerId[] = "microsoft/test-handler";
    entity.DownloadHandlerId = handlerId;

    ADUC_Result result = ProcessDownloadHandlerExtensibility(&dummy, &entity, "/tmp/target");
    CHECK(result.ResultCode == 1);
    CHECK(result.ExtendedResultCode == 0);
}

// =====================================================================
// GetDownloadTimeoutInMinutes tests  
// =====================================================================

TEST_CASE_METHOD(HelperFixture, "Helper: GetDownloadTimeoutInMinutes returns default when config is null")
{
    mock_helper_config_return_null = true;

    unsigned int timeout = GetDownloadTimeoutInMinutes(nullptr);
    CHECK(timeout == CONTENT_DOWNLOADER_MAX_TIMEOUT_IN_MINUTES_DEFAULT);
}

TEST_CASE_METHOD(HelperFixture, "Helper: GetDownloadTimeoutInMinutes uses downloadOptions when config timeout is zero")
{
    mock_helper_config_download_timeout = 0;

    ExtensionManager_Download_Options options{ 42 };
    unsigned int timeout = GetDownloadTimeoutInMinutes(&options);
    CHECK(timeout == 42);
}

TEST_CASE_METHOD(HelperFixture, "Helper: GetDownloadTimeoutInMinutes returns default when config is zero and options is null")
{
    mock_helper_config_download_timeout = 0;

    unsigned int timeout = GetDownloadTimeoutInMinutes(nullptr);
    CHECK(timeout == CONTENT_DOWNLOADER_MAX_TIMEOUT_IN_MINUTES_DEFAULT);
}

TEST_CASE_METHOD(HelperFixture, "Helper: GetDownloadTimeoutInMinutes uses config timeout when non-zero")
{
    mock_helper_config_download_timeout = 120;

    ExtensionManager_Download_Options options{ 42 };
    unsigned int timeout = GetDownloadTimeoutInMinutes(&options);
    CHECK(timeout == 120);
}

TEST_CASE_METHOD(HelperFixture, "Helper: GetDownloadTimeoutInMinutes config override takes precedence over options")
{
    mock_helper_config_download_timeout = 300;

    ExtensionManager_Download_Options options{ 60 };
    unsigned int timeout = GetDownloadTimeoutInMinutes(&options);
    CHECK(timeout == 300);
}

TEST_CASE_METHOD(HelperFixture, "Helper: default timeout constant is 480 minutes")
{
    CHECK(CONTENT_DOWNLOADER_MAX_TIMEOUT_IN_MINUTES_DEFAULT == (8 * 60));
}

TEST_CASE_METHOD(HelperFixture, "Helper: Default_ExtensionManager_Download_Options has default timeout")
{
    CHECK(Default_ExtensionManager_Download_Options.timeoutInMinutes == CONTENT_DOWNLOADER_MAX_TIMEOUT_IN_MINUTES_DEFAULT);
}
