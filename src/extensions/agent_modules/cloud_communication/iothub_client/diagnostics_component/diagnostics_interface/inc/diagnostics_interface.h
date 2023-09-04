/**
 * @file diagnostics_interface.h
 * @brief Methods to communicate with "dtmi:azure:iot:deviceUpdateDiagnosticModel;1" interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef DIAGNOSTICS_INTERFACE_H
#define DIAGNOSTICS_INTERFACE_H

#include <aduc/adu_types.h>
#include <aduc/c_utils.h>
#include <aduc/client_handle.h>
#include <azureiot/iothub_client_core_common.h>
#include <diagnostics_result.h>
#include <parson.h>

#include <stdbool.h>

EXTERN_C_BEGIN

//
// Device to cloud JSON Fields
//
/**
 * @brief JSON field name for the resultCode
 */
#define DIAGNOSTICSITF_FIELDNAME_RESULTCODE "resultCode"

/**
 * @brief JSON field name for the extendedResultCode
 */
#define DIAGNOSTICSITF_FIELDNAME_EXTENDEDRESULTCODE "extendedResultCode"

//
// Service Request JSON ITF
//

/**
 * @brief JSON field name for the storageSasUrl
 */
#define DIAGNOSTICSITF_FIELDNAME_SASURL "storageSasUrl"

/**
 * @brief JSON field name for the operationId
 */
#define DIAGNOSTICSITF_FIELDNAME_OPERATIONID "operationId"

/**
 * @brief Handle for Diagnostics component to communicate to the service.
 */
extern ADUC_ClientHandle g_iotHubClientHandleForDiagnosticsComponent;

//
// Registration/Unregistration
//

/**
 * @brief Initialize the interface.
 *
 * @param[out] componentContext Optional context object to use in related calls.
 * @param argc Count of arguments in @p argv
 * @param argv Command line parameters.
 * @return bool True on success.
 */
bool DiagnosticsInterface_Create(void** componentContext, int argc, char** argv);

/**
 * @brief Called after connected to IoTHub (device client handler is valid).
 *
 * @param context Context object from Create.
 */
void DiagnosticsInterface_Connected(void* componentContext);

/**
 * @brief Uninitialize the interface.
 *
 * @param componentContext Optional context object which was returned from Create.
 */
void DiagnosticsInterface_Destroy(void** componentContext);

/**
 * @brief A callback for the diagnostic component's property update events.
 */
void DiagnosticsInterface_PropertyUpdateCallback(
    ADUC_ClientHandle clientHandle,
    const char* propertyName,
    JSON_Value* propertyValue,
    int version,
    ADUC_PnPComponentClient_PropertyUpdate_Context* sourceContext,
    void* context);

//
// Reporting
//

/**
 * @brief Report a new state to the server.
 * @param result the result to be reported to the service
 */
void DiagnosticsInterface_ReportStateAndResultAsync(const Diagnostics_Result result, const char* operationId);

EXTERN_C_END

#endif // DIAGNOSTICS_INTERFACE_H
