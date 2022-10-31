/**
 * @file adu_core_export_helpers.h
 * @brief Provides set of helpers for creating objects defined in adu_core_exports.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_ADU_CORE_EXPORT_HELPERS_H
#define ADUC_ADU_CORE_EXPORT_HELPERS_H

#include "aduc/adu_core_exports.h"
#include "aduc/c_utils.h"
#include "aduc/types/hash.h" // for ADUC_Hash
#include "aduc/types/update_content.h" // for ADUC_FileEntity
#include "aduc/types/workflow.h"

#include <stdbool.h>

EXTERN_C_BEGIN
//
// Register/Unregister methods.
//

/**
 * @brief Call Unregister method to indicate startup.
 *
 * @param updateActionCallbacks UpdateActionCallbacks object.
 * @param argc Count of arguments in @p argv
 * @param argv Command line parameters.
 */
ADUC_Result
ADUC_MethodCall_Register(ADUC_UpdateActionCallbacks* updateActionCallbacks, unsigned int argc, const char** argv);

/**
 * @brief Call Unregister method to indicate shutdown.
 *
 * @param updateActionCallbacks UpdateActionCallbacks object.
 */
void ADUC_MethodCall_Unregister(const ADUC_UpdateActionCallbacks* updateActionCallbacks);

//
// Reboot system
//

int ADUC_MethodCall_RebootSystem();

//
// Restart the ADU Agent.
//

int ADUC_MethodCall_RestartAgent();

EXTERN_C_END

#endif // ADUC_ADU_CORE_EXPORT_HELPERS_H
