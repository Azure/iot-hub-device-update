
#include "aduc/c_utils.h"
#include <stdlib.h>
#include <stdint.h>

EXTERN_C_BEGIN

/**
 * @brief Handle for communication to service.
 */
extern void* /* ADUC_ClientHandle */ g_iotHubClientHandleForADUComponent;

//
// mock deps of libiothub_client.a archive
//

// mock iothub_client_core_ll.c.o
// refs from fn initialize_iothub_client
#ifndef IOTHUB_AUTHORIZATION_HANDLE
#    define IOTHUB_AUTHORIZATION_HANDLE void*
#endif

#define IOTHUB_AUTHORIZATION_HANDLE void*
IOTHUB_AUTHORIZATION_HANDLE IoTHubClient_Auth_Create(const char* device_key, const char* device_id, const char* sas_token, const char* module_id)
{
    UNREFERENCED_PARAMETER(device_key);
    UNREFERENCED_PARAMETER(device_id);
    UNREFERENCED_PARAMETER(sas_token);
    UNREFERENCED_PARAMETER(module_id);
    return (IOTHUB_AUTHORIZATION_HANDLE)malloc(1);
}

IOTHUB_AUTHORIZATION_HANDLE IoTHubClient_Auth_CreateFromDeviceAuth(const char* device_id, const char* module_id)
{
    UNREFERENCED_PARAMETER(device_id);
    UNREFERENCED_PARAMETER(module_id);
    return (IOTHUB_AUTHORIZATION_HANDLE)malloc(1);
}

int IoTHubClient_Auth_Set_SasToken_Expiry(IOTHUB_AUTHORIZATION_HANDLE handle, uint64_t expiry_time_seconds)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(expiry_time_seconds);
    return 0;
}

void IoTHubClient_Auth_Destroy(IOTHUB_AUTHORIZATION_HANDLE handle)
{
    free(handle);
}

// mock iothub_client_ll_uploadtoblob.c.o
// refs from send_http_sas_request
const char* IoTHubClient_Auth_Get_DeviceKey(IOTHUB_AUTHORIZATION_HANDLE handle)
{
    return "mock_device_key";
}

// refs from IoTHubClient_LL_UploadToBlob_Create
const char* IoTHubClient_Auth_Get_DeviceId(IOTHUB_AUTHORIZATION_HANDLE handle)
{
    return "mock_device_id";
}

#define IOTHUB_CREDENTIAL_TYPE int

IOTHUB_CREDENTIAL_TYPE IoTHubClient_Auth_Get_Credential_Type(IOTHUB_AUTHORIZATION_HANDLE handle)
{
    UNREFERENCED_PARAMETER(handle);
    return 0;
}

int IoTHubClient_Auth_Get_x509_info(IOTHUB_AUTHORIZATION_HANDLE handle, char** x509_cert, char** x509_key)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(x509_cert);
    UNREFERENCED_PARAMETER(x509_key);
    return 0;
}

char* IoTHubClient_Auth_Get_SasToken(IOTHUB_AUTHORIZATION_HANDLE handle, const char* scope, uint64_t expiry_time_relative_seconds)
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(scope);
    UNREFERENCED_PARAMETER(expiry_time_relative_seconds);
    return nullptr;
}

EXTERN_C_END