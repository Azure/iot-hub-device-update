/**
 * @file delta_download_plugin.hpp
 * @brief header for DownloadHandlerPlugin class that abstracts using a download handler extension shared library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef DOWNLOAD_HANDLER_PLUGIN_HPP
#define DOWNLOAD_HANDLER_PLUGIN_HPP

#include <aduc/logging.h> // ADUC_LOG_SEVERITY
#include <aduc/result.h> // ADUC_Result
#include <aduc/shared_lib.hpp> // aduc::SharedLib
#include <aduc/types/update_content.h> // typedef struct ADUC_FileEntity
#include <memory> // std::unique_ptr
#include <string>

using ADUC_WorkflowHandle = void*;

class DownloadHandlerPlugin
{
public:
    DownloadHandlerPlugin(const DownloadHandlerPlugin&) = delete;
    DownloadHandlerPlugin& operator=(const DownloadHandlerPlugin&) = delete;
    DownloadHandlerPlugin(DownloadHandlerPlugin&&) = delete;
    DownloadHandlerPlugin& operator=(DownloadHandlerPlugin&&) = delete;

    DownloadHandlerPlugin(const std::string& downloadHandlerId, ADUC_LOG_SEVERITY logLevel);
    ~DownloadHandlerPlugin();
    ADUC_Result ProcessUpdate(
        const ADUC_WorkflowHandle workflowHandle,
        const ADUC_FileEntity* fileEntity,
        const char* payloadFilePath) const;
    ADUC_Result OnUpdateWorkflowCompleted(const ADUC_WorkflowHandle workflowHandle) const;

private:
    std::unique_ptr<aduc::SharedLib> lib;
};

#endif // DOWNLOAD_HANDLER_PLUGIN_HPP
