/**
 * @file extension_manager_helper.hpp
 * @brief Headers for ExtensionManager helpers.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_EXTENSION_MANAGER_HELPER_HPP
#define ADUC_EXTENSION_MANAGER_HELPER_HPP

#include <aduc/c_utils.h>
#include <aduc/extension_manager_download_options.h>
#include <aduc/result.h>

#include <aduc/types/update_content.h>
using ADUC_WorkflowHandle = void*;
class DownloadHandlerPlugin;

EXTERN_C_BEGIN

ADUC_Result ProcessDownloadHandlerExtensibility(
    ADUC_WorkflowHandle workflowHandle, const ADUC_FileEntity* entity, const char* targetUpdateFilePath) noexcept;

unsigned int GetDownloadTimeoutInMinutes(const ExtensionManager_Download_Options* downloadOptions) noexcept;

/**
 * @brief Returns whether the download timeout ends a download before downloadNoProgressTimeoutInMinutes would.
 * @param noProgressTimeoutInMinutes The configured no-progress timeout, 0 when not configured.
 * @param downloadTimeoutInMinutes The download timeout in effect, see GetDownloadTimeoutInMinutes.
 */
bool IsNoProgressTimeoutCappedByDownloadTimeout(
    unsigned int noProgressTimeoutInMinutes, unsigned int downloadTimeoutInMinutes) noexcept;

/**
 * @brief Logs a warning when downloadNoProgressTimeoutInMinutes is longer than the download timeout, which caps it.
 * @remark This function requires that the ADUC_ConfigInfo singleton has been initialized.
 */
void WarnIfNoProgressTimeoutIsCapped() noexcept;

EXTERN_C_END

#endif // ADUC_EXTENSION_MANAGER_HELPER_HPP
