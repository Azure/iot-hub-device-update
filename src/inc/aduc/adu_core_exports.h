/**
 * @file adu_core_exports.h
 * @brief Describes methods to be exported from platform-specific ADUC agent code.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_ADU_CORE_EXPORTS_H
#define ADUC_ADU_CORE_EXPORTS_H

#include "aduc/c_utils.h"
#include "aduc/result.h"
#include "aduc/types/adu_core.h"
#include <stddef.h>
#include <stdint.h> // uint64_t

EXTERN_C_BEGIN

/**
 * @brief Register this module for callbacks.
 *
 * @param data Information about this module (e.g. callback methods)
 * @param argc Count of optional arguments.
 * @param argv Initialization arguments, size is argc.
 * @return ADUC_Result Result code.
 */
ADUC_Result ADUC_RegisterPlatformLayer(ADUC_UpdateActionCallbacks* data, unsigned int argc, const char** argv);

/**
 * @brief Unregister the callback module.
 *
 * @param token Opaque token that was set in #ADUC_UpdateActionCallbacks.
 */
void ADUC_Unregister(ADUC_Token token);

/**
 * @brief Reboot the system.
 *
 * @returns int errno, 0 if success.
 */
int ADUC_RebootSystem();

/**
 * @brief Restart the ADU Agent.
 *
 * @returns int errno, 0 if success.
 */
int ADUC_RestartAgent();

#define AducResultCodeIndicatesInProgress(resultCode)                                                  \
    ((resultCode) == ADUC_Result_Download_InProgress || (resultCode) == ADUC_Result_Backup_InProgress || \
    (resultCode) == ADUC_Result_Install_InProgress || (resultCode) == ADUC_Result_Apply_InProgress || \
    (resultCode) == ADUC_Result_Restore_InProgress)

EXTERN_C_END

#endif // ADUC_ADU_CORE_EXPORTS_H
