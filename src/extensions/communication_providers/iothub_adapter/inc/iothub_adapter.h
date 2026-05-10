/**
 * @file iothub_adapter.h
 * @brief IoT Hub backward-compatibility adapter for Gen2 communication vtable.
 *
 * Stub implementation that wraps the Gen1 IoT Hub communication manager
 * behind the ADUC_CommunicationVtable interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef IOTHUB_ADAPTER_H
#define IOTHUB_ADAPTER_H

#include "aduc/extension_context.h"
#include "aduc/extension_descriptor.h"
#include "aduc/communication_vtable.h"

#ifdef __cplusplus
extern "C"
{
#endif

ADUC_Result2 IotHubAdapter_Initialize(const ADUC_ExtensionContext* ctx);
void IotHubAdapter_Uninitialize(void);

#ifdef __cplusplus
}
#endif

#endif // IOTHUB_ADAPTER_H
