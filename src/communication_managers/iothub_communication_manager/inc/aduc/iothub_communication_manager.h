/**
 * @file iothub_communication_manager.h
 * @brief Implements the IoT Hub communication manager utility.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef IOT_HUB_COMMUNICATION_MANAGER_H
#define IOT_HUB_COMMUNICATION_MANAGER_H

#include "aduc/c_utils.h"
#include "aduc/client_handle_helper.h"

EXTERN_C_BEGIN

/**
 * @brief A callback function to be invoked when a device client handler has changed.
 *
 * @param client_handle A new device client handler.
 */
typedef void (*ADUC_COMMUNICATION_MANAGER_CLIENT_HANDLE_UPDATED_CALLBACK)(ADUC_ClientHandle client_handle);

/**
 * @brief Initializes the IoT Hub connection manager.
 *
 * @param handle_address A pointer to ADUC_ClientHandle data.
 * @param device_twin_callback A callback function to be invoked when receiving a device twin data.
 * @param client_handle_updated_callback A pointer to a callback function to be invoked when a device client handler has changed.
 * @param property_update_context An ADUC_PnPComponentClient_PropertyUpdate_Context object.
 *
 * @return 'true' if success.
 */
bool IoTHub_CommunicationManager_Init(ADUC_ClientHandle* handle_address,
                                   IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK device_twin_callback,
                                   ADUC_COMMUNICATION_MANAGER_CLIENT_HANDLE_UPDATED_CALLBACK client_handle_updated_callback,
                                   ADUC_PnPComponentClient_PropertyUpdate_Context *property_update_context
                                   );

/**
 * @brief De-initialize the IoT Hub connection manager.
 */
void IoTHub_CommunicationManager_Deinit();

/**
 * @brief Checks whether the connection to IoT Hub is authenticated.
 */
bool IoTHub_CommunicationManager_IsAuthenticated();

/**
 * @brief Gets the current IoT Hub connection handle.
 *
 * @return ADUC_ClientHandle object.
 */
ADUC_ClientHandle IoTHub_CommunicationManager_GetHandle();
/**
 * @brief A callback use for processing the IoT Hub Client connection status changed event.
 *
 * @param status An IoT Hub connection status
 * @param status_reason The value indicates the reason that the IoT Hub connection status change.
 * @param user_context_callback Additional context used for processing this status changed event.
 */
void IoTHub_CommunicationManager_ConnectionStatus_Callback(
    IOTHUB_CLIENT_CONNECTION_STATUS status,
    IOTHUB_CLIENT_CONNECTION_STATUS_REASON status_reason,
    void* user_context_callback);

/**
 * @brief Performs the connection management tasks synchronously (in caller's thread context).
 *
 * @remark This function may destroy the current IoT Hub client handler. Hence, it must not be called while
 *         the IoT Hub client handler is in use.
 */
void IoTHub_CommunicationManager_DoWork(void* user_context);

EXTERN_C_END

#endif // IOT_HUB_COMMUNICATION_MANAGER_H
