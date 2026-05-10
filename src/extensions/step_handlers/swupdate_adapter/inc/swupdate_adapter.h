/**
 * @file swupdate_adapter.h
 * @brief SWUpdate backward-compatibility adapter for Gen2 step handler vtable.
 *
 * Stub implementation that wraps the Gen1 SWUpdate handler behind the
 * ADUC_StepHandlerVtable interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef SWUPDATE_ADAPTER_H
#define SWUPDATE_ADAPTER_H

#include "aduc/extension_context.h"
#include "aduc/extension_descriptor.h"
#include "aduc/step_handler_vtable.h"

#ifdef __cplusplus
extern "C"
{
#endif

ADUC_Result2 SwUpdateAdapter_Initialize(const ADUC_ExtensionContext* ctx);
void SwUpdateAdapter_Uninitialize(void);

#ifdef __cplusplus
}
#endif

#endif // SWUPDATE_ADAPTER_H
