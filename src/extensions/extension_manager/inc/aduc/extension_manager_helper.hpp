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
#include <aduc/result.h>

#include <aduc/types/update_content.h>
using ADUC_WorkflowHandle = void*;
class DownloadHandlerPlugin;

EXTERN_C_BEGIN

ADUC_Result ProcessDownloadHandlerExtensibility(
    ADUC_WorkflowHandle workflowHandle, const ADUC_FileEntity* entity, const char* targetUpdateFilePath) noexcept;

EXTERN_C_END

#endif // ADUC_EXTENSION_MANAGER_HELPER_HPP
