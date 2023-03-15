/**
 * @file eis_utils.c
 * @brief Implements the EIS Utility for connecting to EIS and requesting a provisioned SaS token.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "eis_utils.h"
#include "eis_coms.h"
#include <aduc/string_c_utils.h>
#include <aduc/permission_utils.h>
#include <azure_c_shared_utility/crt_abstractions.h>
#include <azure_c_shared_utility/shared_util_options.h>
#include <azure_c_shared_utility/urlencode.h>
#include <errno.h>
#include <fcntl.h>
#include <parson.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

//
// IdentityService Response FieldNames
//

/**
 * Example Identity Response:
 *
 * {
 *   "type":"aziot",
 *   "spec":{
 *       "hubName":"some-hubname.azure-devices.net",
 *       "gatewayHost":"parentdevice",
 *       "deviceId":"eis-test-device",
 *       "module-id":"some-module-id",
 *       "auth":{
 *           "type":"sas",
 *           "keyHandle":"someKeyHandle"
 *           "certId":"some-cert-id"
 *       }
 *   }
 * }
 *
 */

/**
 * @brief FieldName for the JSON Object which contains the hubName, deviceId, and auth JSON Object
 */
#define EIS_IDENTITY_RESP_SPEC_FIELD "spec"

/**
 * @brief Fieldname for the hubName provisioned to the IdentityService
 */
#define EIS_IDENTITY_RESP_HUBNAME_FIELD "hubName"

/**
 * @brief Fieldname for the gatewayHost provisioned to the IdentityService
 */
#define EIS_IDENTITY_RESP_GATEWAYHOSTNAME_FIELD "gatewayHost"

/**
 * @brief Fieldname for the deviceId provisioned to the IdentityService
 */
#define EIS_IDENTITY_RESP_DEVICEID_FIELD "deviceId"

/**
 * @brief Fieldname for the moduleId provisioned to the IdentityService
 */
#define EIS_IDENTITY_RESP_MODULEID_FIELD "moduleId"

/**
 * @brief Fieldname for the JSON Object which contains the authType and keyHandle
 */
#define EIS_IDENTITY_RESP_AUTH_FIELD "auth"

/**
 * @brief Fieldname for the type returned by the IdentityService
 */
#define EIS_IDENTITY_RESP_AUTH_TYPE_FIELD "type"

/**
 * @brief Fieldname for the certId returned by the IdentityService
 */
#define EIS_IDENTITY_RESP_AUTH_CERTID_FIELD "certId"

/**
 * @brief Fieldname for the keyHandle returned by the IdentityService
 */
#define EIS_IDENTITY_RESP_AUTH_KEYHANDLE_FIELD "keyHandle"

//
// KeyService Sign API Response FieldNames
//

/**
 * Example KeyService Sign Response
 * {
 *   "signature": "hIuFfERqcDBnu84EwVlF01JfiaRvH6A20dMWQW6T4fg="
 * }
 */

/**
 * @brief FieldName for the signature value within the EIS signature response
 */
#define EIS_SIGN_RESP_SIGNATURE_FIELD "signature"

//
// Certificate API Response FieldNames
//

/**
 * Example Certificate Service Response
 * {
 *  "pem":"string"
 * }
 *
 */

/**
 * @brief Fieldname for the returned certificate string in PEM format
 */
#define EIS_CERT_RESP_PEM "pem"

//
// Internal Functions
//

/**
 * @brief Makes a call to the EIS KeyService to sign the @p resourceUri and builds a SharedAccessSignature
 * @param resourceUri the uri to be signed by the EIS KeyService
 * @param keyHandle the handle of the key to be used for signing
 * @param expirySecsSinceEpoch the expiration time for this SharedAccessSignature in Unix Epoch time
 * @param timeoutMS the timeout in milliseconds for the call to the EIS Key Service
 * @returns a value of EISUtilityResult
 */
EISUtilityResult BuildSharedAccessSignature(
    const char* resourceUri,
    const char* keyHandle,
    const time_t expirySecsSinceEpoch,
    uint32_t timeoutMS,
    char** signaturePtr)
{
    EISUtilityResult result = { EISErr_Failed, EISService_Utils };

    bool success = false;
    char* sharedAccessSignature = NULL;
    char* signResponseStr = NULL;
    JSON_Value* signResponseJson = NULL;
    STRING_HANDLE encodedSignature = NULL;

    if (signaturePtr == NULL)
    {
        result.err = EISErr_InvalidArg;
        result.service = EISService_Utils;
        goto done;
    }

    // Note: 12 is the maximum amount of characters needed to represent a time_t in string format
    char expiryStr[ARRAY_SIZE("2147483647")];

    if (snprintf(expiryStr, ARRAY_SIZE(expiryStr), "%ld", expirySecsSinceEpoch) == 0)
    {
        goto done;
    }

    EISErr keyServiceResult = RequestSignatureFromEIS(keyHandle, resourceUri, expiryStr, timeoutMS, &signResponseStr);

    if (keyServiceResult != EISErr_Ok)
    {
        result.service = EISService_KeyService;
        result.err = keyServiceResult;
        goto done;
    }

    signResponseJson = json_parse_string(signResponseStr);

    if (signResponseJson == NULL)
    {
        result.service = EISService_KeyService;
        result.err = EISErr_InvalidJsonRespErr;
        goto done;
    }

    const JSON_Object* signResponseJsonObj = json_value_get_object(signResponseJson);

    if (signResponseJsonObj == NULL)
    {
        result.service = EISService_KeyService;
        result.err = EISErr_InvalidJsonRespErr;
        goto done;
    }

    const char* signature = json_object_get_string(signResponseJsonObj, EIS_SIGN_RESP_SIGNATURE_FIELD);

    if (signature == NULL)
    {
        result.service = EISService_KeyService;
        result.err = EISErr_InvalidJsonRespErr;
        goto done;
    }

    encodedSignature = URL_EncodeString(signature);

    if (encodedSignature == NULL)
    {
        goto done;
    }

    sharedAccessSignature = ADUC_StringFormat(
        "SharedAccessSignature sr=%s&sig=%s&se=%s", resourceUri, STRING_c_str(encodedSignature), expiryStr);

    if (sharedAccessSignature == NULL)
    {
        goto done;
    }

    result.err = EISErr_Ok;
    success = true;

done:

    json_value_free(signResponseJson);

    free(signResponseStr);

    STRING_delete(encodedSignature);

    if (!success)
    {
        free(sharedAccessSignature);
        sharedAccessSignature = NULL;
    }

    *signaturePtr = sharedAccessSignature;

    return result;
}

/**
 * @brief Builds a connection string with the specified SharedAccessSignature as the authentication method
 * @param hubName the hubName for the connection string
 * @param deviceId the device identity for the connection string
 * @param moduleId an optional parameter specifying the module identity to use in the connection string
 * @param connType the connection type being used, if EISConnType_ModuleId then moduleId must not be NULL
 * @param sharedAccessSignature the sharedAccessSignature generated for this connection string
 * @param gatewayHostName the gatewayHostName for the parent device for MCC and Nested Edge scenarios
 * @param connectionStrPtr the pointer to the buffer which will be allocated for the connection string
 * @returns a value of EISErr
 */
EISErr BuildSasTokenConnectionString(
    const char* hubName,
    const char* deviceId,
    const char* moduleId,
    const ADUC_ConnType connType,
    const char* sharedAccessSignature,
    const char* gatewayHostName,
    char** connectionStrPtr)
{
    EISErr result = EISErr_Failed;
    bool success = false;

    char* connectionStr = NULL;

    if (hubName == NULL || deviceId == NULL || connType == ADUC_ConnType_NotSet || sharedAccessSignature == NULL
        || connectionStrPtr == NULL)
    {
        result = EISErr_InvalidArg;
        goto done;
    }

    if (connType == ADUC_ConnType_Device)
    {
        if (gatewayHostName != NULL)
        {
            connectionStr = ADUC_StringFormat(
                "HostName=%s;DeviceId=%s;SharedAccessSignature=%s;GatewayHostName=%s",
                hubName,
                deviceId,
                sharedAccessSignature,
                gatewayHostName);
        }
        else
        {
            connectionStr = ADUC_StringFormat(
                "HostName=%s;DeviceId=%s;SharedAccessSignature=%s", hubName, deviceId, sharedAccessSignature);
        }
    }
    else if (connType == ADUC_ConnType_Module)
    {
        if (moduleId == NULL)
        {
            result = EISErr_InvalidArg;
            goto done;
        }

        if (gatewayHostName != NULL)
        {
            connectionStr = ADUC_StringFormat(
                "HostName=%s;DeviceId=%s;ModuleId=%s;SharedAccessSignature=%s;GatewayHostName=%s",
                hubName,
                deviceId,
                moduleId,
                sharedAccessSignature,
                gatewayHostName);
        }
        else
        {
            connectionStr = ADUC_StringFormat(
                "HostName=%s;DeviceId=%s;ModuleId=%s;SharedAccessSignature=%s",
                hubName,
                deviceId,
                moduleId,
                sharedAccessSignature);
        }
    }
    else
    {
        goto done;
    }

    result = EISErr_Ok;
    success = true;

done:

    if (!success)
    {
        free(connectionStr);
        connectionStr = NULL;
    }

    *connectionStrPtr = connectionStr;

    return result;
}

/**
 * @brief Builds a connection string with the specified a x509 certificate as the authentication method
 * @param hubName the hubName for the connection string
 * @param deviceId the device identity for the connection string
 * @param moduleId an optional parameter specifying the module identity to use in the connection string
 * @param connType the connection type being used, if EISConnType_ModuleId then moduleId must not be NULL
 * @param gatewayHostName the gatewayHostName for the parent device for MCC and Nested Edge scenarios
 * @param connectionStrPtr the pointer to the buffer which will be allocated for the connection string
 * @returns a value of EISErr
 */
EISErr BuildSasCertConnectionString(
    const char* hubName,
    const char* deviceId,
    const char* moduleId,
    const ADUC_ConnType connType,
    const char* gatewayHostName,
    char** connectionStrPtr)
{
    bool success = false;
    EISErr result = EISErr_Failed;

    char* connectionStr = NULL;

    if (hubName == NULL || deviceId == NULL || connType == ADUC_ConnType_NotSet || connectionStrPtr == NULL)
    {
        result = EISErr_InvalidArg;
        goto done;
    }

    if (connType == ADUC_ConnType_Device)
    {
        if (gatewayHostName != NULL)
        {
            connectionStr = ADUC_StringFormat(
                "HostName=%s;DeviceId=%s;x509=true;GatewayHostName=%s", hubName, deviceId, gatewayHostName);
        }
        else
        {
            connectionStr = ADUC_StringFormat("HostName=%s;DeviceId=%s;x509=true", hubName, deviceId);
        }
    }
    else if (connType == ADUC_ConnType_Module)
    {
        if (moduleId == NULL)
        {
            result = EISErr_InvalidArg;
            goto done;
        }

        if (gatewayHostName != NULL)
        {
            connectionStr = ADUC_StringFormat(
                "HostName=%s;DeviceId=%s;ModuleId=%s;x509=true;GatewayHostName=%s",
                hubName,
                deviceId,
                moduleId,
                gatewayHostName);
        }
        else
        {
            connectionStr =
                ADUC_StringFormat("HostName=%s;DeviceId=%s;ModuleId=%s;x509=true", hubName, deviceId, moduleId);
        }
    }
    else
    {
        success = false;
        goto done;
    }

    result = EISErr_Ok;
    success = true;

done:

    if (!success)
    {
        free(connectionStr);
        connectionStr = NULL;
    }

    *connectionStrPtr = connectionStr;

    return result;
}

/**
 * @brief Writing EIS Identities information to a EIS data file
 * @details Calls into the EIS Identity and get the identities response, saves to a file
 * @param[in] timeoutMS the timeoutMS in milliseconds for each call to EIS
 * returns EISErr value
 */
EISErr EISIdentitiesPipeWriter(uint32_t timeoutMS, const char* pipePath)
{
    char* identityResponseStr;

    EISErr retCode = EISErr_Ok;

    EISErr identityResult = RequestIdentitiesFromEIS(timeoutMS, &identityResponseStr);

    if (identityResponseStr != NULL)
    {
        int pipeFd = open(pipePath, O_WRONLY);
        if (pipeFd < 0)
        {
            retCode = EISErr_NamedPipeFailure;
            goto done;
        }
        // dprintf(pipeFd, "%s\n", identityResponseStr);
        // dprintf(pipeFd, "%d\n", identityResult);
        write(pipeFd, identityResponseStr, strlen(identityResponseStr));
        write(pipeFd, "\n", 1);

        char identityResultStr[32];
        snprintf(identityResultStr, sizeof(identityResultStr), "%d\n", identityResult);
        write(pipeFd, identityResultStr, strlen(identityResultStr));

        close(pipeFd);

        printf("Connection data written to named pipe %s.\n", pipePath);
    }
    else
    {
        retCode = EISErr_ConnErr;
    }
done:
    return retCode;
}

/**
 * @brief Reads EIS Identities information from a EIS data file
 * @details Reads EIS Identity from a file, first line being the identity response, 
 * and the second line being the EIS Error Code.
 * @param[in,out] identityResponseBuffer the pointer to Identity response string
 * @param[in,out] childErrno the errno to bubble up to the caller
 * @returns a value of EISErr
 */
EISErr EISIdentitiesPipeReader(const char* pipePath, char** identityResponseBuffer, int* childErrno)
{
    EISErr result=0;
    int pipeFd = open(pipePath, O_RDONLY);
    if (pipeFd < 0)
    {
        *childErrno = errno;
        return result;
    }

    *identityResponseBuffer = malloc(1024 * sizeof(char));
    if (*identityResponseBuffer == NULL) {
        *childErrno = errno;
        goto done;
    }
    
    if (read(pipeFd, *identityResponseBuffer, 1024) < 0) {
        *childErrno = errno;
        free(*identityResponseBuffer);
        goto done;
    }

    char readidentityResult[1024];
    if (read(pipeFd, readidentityResult, 1024) < 0) {
        *childErrno = errno;
        free(*identityResponseBuffer);
        goto done;
    }

    int readidentityResultNum = atoi(readidentityResult);
    result = (EISErr)readidentityResultNum;

done:
    close(pipeFd);
    return result;
}

/**
 * @brief Processes the identity response from EIS and populates provisioningInfo
 * @details Parses the identity response JSON string and extracts the necessary information
 * to create a connection string or certificate using EIS services. The provisioningInfo
 * struct is populated with the extracted information.
 * @param[out] provisioningInfo the pointer to the struct which will be initialized with the information for creating a connection to IoTHub using the EIS supported provisioning information
 * @param[in] identityResponseStr the JSON string containing the identity response from EIS
 * @param[in] expirySecsSinceEpoch the expiration time in seconds since the epoch for the token in the connection string
 * @param[in] timeoutMS the timeout in milliseconds for each call to EIS
 * @returns EISErr_Ok on success, otherwise an error code indicating the type of failure
 */
EISUtilityResult ProcessIdentityResponse(ADUC_ConnectionInfo* provisioningInfo, const char* identityResponseStr, const time_t expirySecsSinceEpoch, uint32_t timeoutMS)
{
    EISUtilityResult result = { EISErr_Failed, EISService_Utils };

    memset(provisioningInfo, 0, sizeof(*provisioningInfo));

    bool success = false;

    char* connectionStr = NULL;

    char* keyHandlePtr = NULL;
    ADUC_ConnType connType = ADUC_ConnType_NotSet;
    ADUC_AuthType authType = ADUC_AuthType_NotSet;

    char* resourceUri = NULL;
    char* sharedSignatureStr = NULL;

    JSON_Value* identityResponseJson = NULL;

    char* certResponseStr = NULL;
    JSON_Value* certResponseJson = NULL;
    char* certString = NULL;

    identityResponseJson = json_parse_string(identityResponseStr);

    if (identityResponseJson == NULL)
    {
        result.err = EISErr_InvalidJsonRespErr;
        result.service = EISService_IdentityService;
        goto done;
    }

    const JSON_Object* identityResponseJsonObj = json_value_get_object(identityResponseJson);

    if (identityResponseJsonObj == NULL)
    {
        goto done;
    }

    const JSON_Object* specJson =
        json_value_get_object(json_object_get_value(identityResponseJsonObj, EIS_IDENTITY_RESP_SPEC_FIELD));

    if (specJson == NULL)
    {
        result.err = EISErr_InvalidJsonRespErr;
        result.service = EISService_IdentityService;
        goto done;
    }

    const char* hubName = json_object_get_string(specJson, EIS_IDENTITY_RESP_HUBNAME_FIELD);

    if (hubName == NULL)
    {
        result.err = EISErr_InvalidJsonRespErr;
        result.service = EISService_IdentityService;
        goto done;
    }

    const char* deviceId = json_object_get_string(specJson, EIS_IDENTITY_RESP_DEVICEID_FIELD);

    if (deviceId == NULL)
    {
        result.err = EISErr_InvalidJsonRespErr;
        result.service = EISService_IdentityService;
        goto done;
    }

    connType = ADUC_ConnType_Device;

    const char* moduleId = json_object_get_string(specJson, EIS_IDENTITY_RESP_MODULEID_FIELD);

    if (moduleId != NULL)
    {
        connType = ADUC_ConnType_Module;
    }

    // Build request for the signature
    if (connType == ADUC_ConnType_Device)
    {
        resourceUri = ADUC_StringFormat("%s/devices/%s", hubName, deviceId);
    }
    else if (connType == ADUC_ConnType_Module)
    {
        resourceUri = ADUC_StringFormat("%s/devices/%s/modules/%s", hubName, deviceId, moduleId);
    }
    else
    {
        goto done;
    }

    if (resourceUri == NULL)
    {
        goto done;
    }

    const char* gatewayHostName = json_object_get_string(specJson, EIS_IDENTITY_RESP_GATEWAYHOSTNAME_FIELD);

    const JSON_Object* authJson =
        json_value_get_object(json_object_get_value(specJson, EIS_IDENTITY_RESP_AUTH_FIELD));

    if (authJson == NULL)
    {
        result.err = EISErr_InvalidJsonRespErr;
        result.service = EISService_IdentityService;
        goto done;
    }

    const char* authTypeStr = json_object_get_string(authJson, EIS_IDENTITY_RESP_AUTH_TYPE_FIELD);

    if (authTypeStr == NULL)
    {
        result.err = EISErr_InvalidJsonRespErr;
        result.service = EISService_IdentityService;
        goto done;
    }

    const char* keyHandle = json_object_get_string(authJson, EIS_IDENTITY_RESP_AUTH_KEYHANDLE_FIELD);

    if (keyHandle == NULL)
    {
        result.err = EISErr_InvalidJsonRespErr;
        result.service = EISService_KeyService;
        goto done;
    }

    if (strcmp(authTypeStr, "sas") == 0)
    {
        authType = ADUC_AuthType_SASToken;

        result = BuildSharedAccessSignature(
            resourceUri, keyHandle, expirySecsSinceEpoch, timeoutMS, &sharedSignatureStr);

        if (result.err != EISErr_Ok)
        {
            goto done;
        }

        result.err = BuildSasTokenConnectionString(
            hubName, deviceId, moduleId, connType, sharedSignatureStr, gatewayHostName, &connectionStr);

        if (result.err != EISErr_Ok)
        {
            goto done;
        }
    }
    else if (strcmp(authTypeStr, "x509") == 0)
    {
        authType = ADUC_AuthType_SASCert;

        if (mallocAndStrcpy_s(&keyHandlePtr, keyHandle) != 0)
        {
            result.err = EISErr_ContentAllocErr;
            goto done;
        }

        const char* certId = json_object_get_string(authJson, EIS_IDENTITY_RESP_AUTH_CERTID_FIELD);

        if (certId == NULL)
        {
            result.err = EISErr_InvalidJsonRespErr;
            result.service = EISService_IdentityService;
            goto done;
        }

        EISErr certResult = RequestCertificateFromEIS(certId, timeoutMS, &certResponseStr);

        if (certResult != EISErr_Ok)
        {
            result.err = certResult;
            result.service = EISService_CertService;
            goto done;
        }

        certResponseJson = json_parse_string(certResponseStr);

        if (certResponseJson == NULL)
        {
            result.err = EISErr_InvalidJsonRespErr;
            result.service = EISService_CertService;
            goto done;
        }

        const JSON_Object* certResponseJsonObj = json_value_get_object(certResponseJson);

        const char* certificateStr = json_object_get_string(certResponseJsonObj, EIS_CERT_RESP_PEM);

        if (certificateStr == NULL)
        {
            result.err = EISErr_InvalidJsonRespErr;
            result.service = EISService_CertService;
            goto done;
        }

        if (mallocAndStrcpy_s(&certString, certificateStr) != 0)
        {
            result.err = EISErr_ContentAllocErr;
            goto done;
        }

        result.err =
            BuildSasCertConnectionString(hubName, deviceId, moduleId, connType, gatewayHostName, &connectionStr);

        if (result.err != EISErr_Ok)
        {
            goto done;
        }
    }
    else
    {
        // Authentication type not supported
        result.err = EISErr_RecvInvalidValueErr;
        result.service = EISService_IdentityService;
        goto done;
    }

    success = true;
    result.err = EISErr_Ok;

done:
    json_value_free(identityResponseJson);

    free(resourceUri);

    free(sharedSignatureStr);

    provisioningInfo->connectionString = connectionStr;
    provisioningInfo->certificateString = certString;
    provisioningInfo->opensslPrivateKey = keyHandlePtr;
    provisioningInfo->connType = connType;
    provisioningInfo->authType = authType;

    if (provisioningInfo->authType == ADUC_AuthType_SASCert && provisioningInfo->certificateString != NULL)
    {
        if (mallocAndStrcpy_s(&provisioningInfo->opensslEngine, EIS_OPENSSL_KEY_ENGINE_ID) != 0)
        {
            success = false;
        }
    }

    if (!success)
    {
        ADUC_ConnectionInfo_DeAlloc(provisioningInfo);
    }

    return result;
}

//
// External Functions
//

/**
 * @brief Creates a connection string using the provisioned data within EIS
 * @details Calls into the EIS Identity and Keyservice to create a SharedAccessSignature which is then used
 * to create the connection string, Caller is required to call free() to deallocate the connection string
 * @param[in] expirySecsSinceEpoch the expiration time in seconds since the epoch for the token in the connection string
 * @param[in] timeoutMS the timeoutMS in milliseconds for each call to EIS
 * @param[out] provisioningInfo the pointer to the struct which will be initialized with the information for creating a connection to IotHub using the EIS supported provisioning information
 * @returns on success a null terminated connection string, otherwise NULL
 */
EISUtilityResult RequestConnectionStringFromEISWithExpiry(
    const time_t expirySecsSinceEpoch, uint32_t timeoutMS, ADUC_ConnectionInfo* provisioningInfo)
{
    EISUtilityResult result = { EISErr_Failed, EISService_Utils };

    if (provisioningInfo == NULL)
    {
        result.err = EISErr_InvalidArg;
        return result;
    }

    if (expirySecsSinceEpoch <= time(NULL) || timeoutMS == 0)
    {
        result.err = EISErr_InvalidArg;
        return result;
    }

    char* identityResponseStr = NULL;


    printf("Parent process (PID %d) is about to fork a child process...\n", getpid());

    // Create a named pipe (FIFO)
    if (mkfifo(EIS_PIPE_PATH, 0666) == -1) {
        result.err = EISErr_NamedPipeFailure;
        goto done;
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        result.err = errno;
        goto done;
    }
    else if (pid == 0)
    {
        // child process
        printf("Child process (PID %d) running...\n", getpid());

        if (!PermissionUtils_SetProcessEffectiveUID(EIS_USER))
        {
            result.err = errno;
            goto done;
        }

        printf("Current UID calling Azure Identity Service: %d\n", getuid());

        EISErr errorCode = EISIdentitiesPipeWriter(timeoutMS, EIS_PIPE_PATH);
        if (errorCode == EISErr_Ok) {
            exit(0);
        } else if (errorCode == EISErr_Failed) {
            exit(1);
        } else {
            exit(errorCode);
        }
    }
    else
    { // parent process
        int status;
        waitpid(pid, &status, 0); // wait for child process to exit

        printf("Parent process (PID %d) reading data from named pipe...\n", getpid());

        int childErrno;
        EISErr EISResult = EISIdentitiesPipeReader(EIS_PIPE_PATH, &identityResponseStr, &childErrno);

        if (EISResult != EISErr_Ok)
        {
            result.service = EISService_IdentityService;
            result.err = EISResult;
            errno = childErrno; // Set the errno to the value received from the child process
            goto done;
        }

        result = ProcessIdentityResponse(provisioningInfo, identityResponseStr, expirySecsSinceEpoch, timeoutMS);

    }
done:
    free(identityResponseStr);
    return result;
}