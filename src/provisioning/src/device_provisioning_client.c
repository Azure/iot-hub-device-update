
/**
 * @file device_provisioning_client.c
 * @brief Implementation for device provisioning client for ADUC.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/device_provisioning_client.h"

#include <aduc/logging.h>
#include <azure_c_shared_utility/threadapi.h>
#include <azure_prov_client/prov_device_client.h>
#include <azure_prov_client/prov_security_factory.h> // for SECURE_DEVICE_TYPE, prov_dev_security_init
#include <stdbool.h>

volatile static bool g_registration_complete = false;

typedef struct tagProvUserContext
{
    PROV_DEVICE_LL_HANDLE handle;
    char* iothub_uri;
    char* device_id;
    bool registration_complete;
} ProvUserContext;

static void register_device_callback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
    user_context;
    if (register_result == PROV_DEVICE_RESULT_OK)
    {
        Log_Info("Registration Information received from service: %s, deviceId: %s", iothub_uri, device_id);
    }
    else
    {
        Log_Error("Failure registering device: %s", MU_ENUM_TO_STRING(PROV_DEVICE_RESULT, register_result));
    }

    g_registration_complete = true;

    if (user_context == NULL)
    {
        Log_Error("user_context is NULL");
    }
    else
    {
        ProvUserContext* user_ctx = (ProvUserContext*)user_context;
        user_ctx->registration_complete = true;
        if (register_result == PROV_DEVICE_RESULT_OK)
        {
            Log_Info("Registration Information received from service: %s", iothub_uri);
            if (mallocAndStrcpy_s(&user_ctx->iothub_uri, iothub_uri) != 0 ||
                mallocAndStrcpy_s(&user_ctx->device_id, device_id) != 0)
            {
                Log_Error("Unable to alloc+cpy iothub_uri or device_id");
            }
        }
        else
        {
            Log_Error("Failure encountered on registration %s", MU_ENUM_TO_STRING(PROV_DEVICE_RESULT, register_result));
        }
    }
}

static void registration_status_callback(PROV_DEVICE_REG_STATUS reg_status, void* user_context)
{
    AZURE_UNREFERENCED_PARAMETER(user_context);
    Log_Info("Provisioning Status: %s", MU_ENUM_TO_STRING(PROV_DEVICE_REG_STATUS, reg_status));
}

static ADUC_Result GetSymmetricKeyInfo(STRING_HANDLE* outRegistration_id, char** outKey)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure, 0 };
    char[] key = ""; // TODO: Get from HSM

    // TODO: Extensibility point--Ideally this is coming straight out of an HSM instead of just a file.
    *outRegistration_id = STRING_construct("my_registration_id");

    if (MallocAndStrcpy_s(outKey, key) != 0)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    return result;
}

static ADUC_Result InitSymmetricAttestationInfo()
{
    ADUC_Result result = { ADUC_GeneralResult_Failure, 0 };
    STRING_HANDLE symmetric_registration_id = NULL;
    char* symmetric_key = NULL;

    // Set the symmetric key if using that auth type
    // If using DPS with an enrollment group, this must be the derived device key from the DPS Primary Key
    // https://docs.microsoft.com/azure/iot-dps/concepts-symmetric-key-attestation?tabs=azure-cli#group-enrollments
    if (Get_Symmetric_Info(&symmetric_registration_id, &symmetric_key) != 0)
    {
        Log_Error("Failed to get symmetric info.");
        result.ExtendedResultCode = ADUC_ERC_DEVICE_PROVISIONING_GET_SYMMETRIC_INFO;
        goto done;
    }

    prov_dev_set_symmetric_key_info(STRING_c_str(symmetric_registration_id), STRING_c_str(symmetric_key));

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    STRING_delete(symmetric_registration_id);

    // Need to make it more secure by ensuring the zeroing out code is not compiled-out so that it reduces
    // window of opportunity of it ending up in the hibernation/swapfile or crash dump file.
    // TODO: Find equivalent for *nix OS.
    SecureZeroMemory(symmetric_key, strlen(symmetric_key));
    free(symmetric_key);

    return result;
}

/**
 * @brief Retrieves the connection string from the device provisioning service with identity attestation authentication with a Device Provisioning instance.
 *
 * @param options The provisioning options.
 * @param outConnectionString The output connection string.
 * @details On success, caller is responsible for calling STRING_delete on the @p outConnectionString STRING_HANDLE
 *     PRE-CONDITION: IoTHub_Init() was called before calling this.
 * @return ADUC_Result The result.
 */
ADUC_Result ADUC_DeviceProvisioning_GetConnectionString(const ADUC_Provisioning_Options options, STRING_HANDLE* outConnectionString)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    ADUC_Result resultInitSymmetric = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    SECURE_DEVICE_TYPE hsm_type = SECURE_DEVICE_TYPE_SYMMETRIC_KEY;

    STRING_HANDLE symmetric_registration_id = NULL;
    STRING_HANDLE symmetric_key = NULL;

    PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION prov_transport;
    HTTP_PROXY_OPTIONS http_proxy;
    memset(&http_proxy, 0, sizeof(HTTP_PROXY_OPTIONS));

    PROV_DEVICE_RESULT prov_device_result = PROV_DEVICE_RESULT_ERROR;
    PROV_DEVICE_HANDLE prov_device_handle;

    ProvUserContext user_context;
    memset(&user_context, 0, sizeof(user_context));

    bool security_inited = false;

    if (prov_dev_security_init(hsm_type) != 0)
    {
        result.ExtendedResultCode = ADUC_ERC_DEVICE_PROVISIONING_PROV_SECURITY_INIT;
        goto done;
    }

    security_inited = true;

    resultInitSymmetric = InitSymmetricAttestationInfo();
    if (IsAducResultCodeFailure(resultInitSymmetric.ResultCode))
    {
        result = resultInitSymmetric;
        goto done;
    }

    // Only MQTT_WS and MQTT will be supported.
    if (options.useWebSockets)
    {
        prov_transport = Prov_Device_MQTT_WS_Protocol;
    }
    else
    {
        prov_transport = Prov_Device_MQTT_Protocol;
    }

    Log_Info("Provisioning API Version: %s\r\n", Prov_Device_GetVersionString());

    if (options.proxy_address != NULL && options.proxy_port > 0)
    {
        http_proxy.host_address = options.proxy_address;
        http_proxy.port = options.proxy_port;
    }

    if ((prov_device_handle = Prov_Device_Create(options.prov_uri, id_scope, prov_transport)) == NULL)
    {
        Log_Error("Prov_Device_Create failed");
        result.ExtendedResultCode = ADUC_ERC_DEVICE_PROVISIONING_PROV_DEVICE_CREATE;
        goto done;
    }
    else
    {
        if (http_proxy.host_address != NULL)
        {
            Prov_Device_SetOption(prov_device_handle, OPTION_HTTP_PROXY, &http_proxy);
        }

        //bool traceOn = true;
        //Prov_Device_SetOption(prov_device_handle, PROV_OPTION_LOG_TRACE, &traceOn);

        // Setting the Trusted Certificate. This is only necessary on systems without
        // built in certificate stores.
        // Prov_Device_SetOption(prov_device_handle, OPTION_TRUSTED_CERT, certificates);

        // This option sets the registration ID it overrides the registration ID that is
        // set within the HSM so be cautious if setting this value
        //Prov_Device_SetOption(prov_device_handle, PROV_REGISTRATION_ID, "[REGISTRATION ID]");

        prov_device_result = Prov_Device_Register_Device(prov_device_handle, register_device_callback, user_context, registration_status_callback, NULL);

        unsigned int max_time = get_cur_time() + options.max_timeout_seconds;
        if (prov_device_result == PROV_DEVICE_RESULT_OK)
        {
            Log_Debug("Registering Device");
            do
            {
                ThreadAPI_Sleep(1000);
                if (get_cur_time() > max_time)
                {
                    Log_Error("Device registration timed out");
                    result.ExtendedResultCode = ADUC_ERC_DEVICE_PROVISIONING_PROV_DEVICE_CREATE;
                    goto done;
                }
            } while (!g_registration_complete);
        }
        else
        {
            Log_Error("Registering failed with error: %d", prov_device_result);
            result.ExtendedResultCode = MAKE_DEVICE_PROVISIONING_ERC(prov_device_result);
            goto done;
        }
    }

    *outConnectionString = STRING_construct(g_iothub_uri);
    if (*outConnectionString == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;

done:
    if (prov_device_handle != NULL)
    {
        Prov_Device_Destroy(prov_device_handle);
    }

    if (security_inited)
    {
        prov_dev_security_deinit();
    }

    return result;
}
