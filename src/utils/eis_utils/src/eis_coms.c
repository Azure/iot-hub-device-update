/**
 * @file eis_coms.c
 * @brief Implements the HTTP communication with EIS over UDS
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "eis_coms.h"

#include <aduc/string_c_utils.h>
#include <azure_c_shared_utility/azure_base64.h>
#include <azure_c_shared_utility/buffer_.h>
#include <azure_c_shared_utility/shared_util_options.h>
#include <azure_c_shared_utility/socketio.h>
#include <azure_c_shared_utility/urlencode.h>
#include <azure_uhttp_c/uhttp.h>
#include <parson.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <umock_c/umock_c_prod.h>

#ifdef ENABLE_MOCKS
#    include "umock_c/umock_c_prod.h"
#endif

//
// EIS UDS Socket Definitions
//

/**
 * @brief Unix Domain Socket (UDS) for the Identity Service API
 */
#define EIS_UDS_IDENTITY_SOCKET_PATH "/run/aziot/identityd.sock"

/**
 * @brief Unix Domain Socket (UDS) for the KeyServices API
 */
#define EIS_UDS_SIGN_SOCKET_PATH "/run/aziot/keyd.sock"

/**
 * @brief Unix Domain Socket (UDS) for the Certificate API
 */
#define EIS_UDS_CERT_SOCKET_PATH "/run/aziot/certd.sock"

/**
 * @brief EIS API version for all calls to EIS
 */
#define EIS_API_VERSION "api-version=2020-09-01"

//
// EIS HTTP Definitions
//

/**
 * @brief URI on the IdentityService UDS Socket for the identity API
 */
#define EIS_IDENTITY_URI "http://foo/identities/identity"

/**
 * @brief URI that is required for a successful call to the identity service
 */
#define EIS_IDENTITY_REQUEST_URI EIS_IDENTITY_URI "?" EIS_API_VERSION

/**
 * @brief URI on the KeyService UDS Socket for the sign API
 */
#define EIS_SIGN_URI "/sign"

/**
 * @brief URI that is required for a successful call to the key service
 */
#define EIS_SIGN_REQUEST_URI EIS_SIGN_URI "?" EIS_API_VERSION

/**
 * @brief URI for the Certificate Service on the Certificate Service's Unix Domain Socket (UDS)
 */
#define EIS_CERT_URI "http://foo/certificates"

//
// KeyService Sign API Request FieldNames
//

/**
 * Example KeyService Sign Request
 * {
 *  "keyHandle":"foo",
 *  "algorithm":"HMAC-256",
 *  "parameters" : {
 *      "message":"something-to-be-signed"
 *  }
 * }
 */

/**
 * @brief Fieldname for the keyHandle which the KeyService sign API uses to find the key for the signature
 */
#define EIS_SIGN_REQ_KEYHANDLE_FIELD "keyHandle"

/**
 * @brief Fieldname for the algorithm to be used to sign the message being sent to the KeyService API
 */
#define EIS_SIGN_REQ_ALG_FIELD "algorithm"

/**
 * @brief Fieldname for the JSON Object which contains the message to be signed by the KeyService sign API
 */
#define EIS_SIGN_REQ_PARAMS_FIELD "parameters"

/**
 * @brief Fieldname for the URI to be signed by the KeyService sign API in dotset format
 */
#define EIS_SIGN_REQ_DOTSET_PARAMS_MSG_FIELD "parameters.message"

/**
 * @brief Algorithm used for the EIS KeyService signature generation
 */
#define EIS_SIGN_ALGORITHM "HMAC-SHA256"

//
// HTTP Control Structures
//

/**
 * @brief Workload context for the calls to EIS
 */
typedef struct tagEIS_HTTP_WORKLOAD_CONTEXT
{
    bool continue_running; //!< Flag indicating that the HTTP call is still in progress
    BUFFER_HANDLE http_response; //!< HTTP Response from EIS
    EISErr status; //!< Status of the HTTP response, this is for connecting, receiving, and errors
} EIS_HTTP_WORKLOAD_CONTEXT;

/**
 * @brief Minimum amount of bytes for any EIS response
 */
#define EIS_RESP_SIZE_MIN 16

/**
 * @brief Maximum amount of bytes for any EIS response
 */
#define EIS_RESP_SIZE_MAX 4096

//
// HTTP Functions
//

/**
 * @brief Error callback for the HTTP connection
 * @param callbackCtx an EIS_HTTP_WORKLOAD_CONTEXT struct that contains the information for the call
 * @param error_result the error returned by the HTTP service
 */
static void on_eis_http_error(void* callbackCtx, HTTP_CALLBACK_REASON error_result)
{
    UNREFERENCED_PARAMETER(error_result);

    EIS_HTTP_WORKLOAD_CONTEXT* workloadCtx = (EIS_HTTP_WORKLOAD_CONTEXT*)callbackCtx;

    if (workloadCtx == NULL)
    {
        return;
    }

    workloadCtx->continue_running = false;
    workloadCtx->status = EISErr_HTTPErr;
}
/**
 * @param callbackCtx an EIS_HTTP_WORKLOAD_CONTEXT struct that contains the information for the call
 * @param requestResult the result of the HTTP call
 * @param content content returned by the HTTP call, for instance a response from a GET call
 * @param contentSize the size of @p content
 * @param statusCode the status code for the HTTP response (e.g. 404, 500, 200, etc.)
 * @param responseHeaders header of the response, used in some calls but not needed here
 *
 */
static void on_eis_http_recv(
    void* callbackCtx,
    HTTP_CALLBACK_REASON requestResult,
    const unsigned char* content,
    size_t contentSize,
    unsigned int statusCode,
    HTTP_HEADERS_HANDLE responseHeaders)
{
    EIS_HTTP_WORKLOAD_CONTEXT* workloadCtx = (EIS_HTTP_WORKLOAD_CONTEXT*)callbackCtx;

    if (workloadCtx == NULL)
    {
        return;
    }

    if (requestResult != HTTP_CALLBACK_REASON_OK || statusCode >= 300 || content == NULL)
    {
        workloadCtx->status = EISErr_HTTPErr;
        goto done;
    }

    if (contentSize < EIS_RESP_SIZE_MIN || contentSize > EIS_RESP_SIZE_MAX)
    {
        workloadCtx->status = EISErr_RecvRespOutOfLimitsErr;
        goto done;
    }

    const char* contentType = HTTPHeaders_FindHeaderValue(responseHeaders, "content-type");

    if (contentType == NULL || strcmp(contentType, "application/json") != 0)
    {
        workloadCtx->status = EISErr_RecvInvalidValueErr;
        goto done;
    }

    workloadCtx->http_response = BUFFER_create(content, contentSize);

    if (workloadCtx->http_response == NULL)
    {
        workloadCtx->status = EISErr_ContentAllocErr;
        goto done;
    }
    else
    {
        workloadCtx->status = EISErr_Ok;
        goto done;
    }

done:

    workloadCtx->continue_running = false;
}

/**
 * @brief Callback for when a connection is accepted or rejected by the HTTP service
 * @param callbackCtx an EIS_HTTP_WORKLOAD_CONTEXT struct that contains the information for the call
 * @param connectResult result of the connection call
 */
static void on_eis_http_connected(void* callbackCtx, HTTP_CALLBACK_REASON connectResult)
{
    EIS_HTTP_WORKLOAD_CONTEXT* workloadCtx = (EIS_HTTP_WORKLOAD_CONTEXT*)callbackCtx;

    if (workloadCtx == NULL)
    {
        return;
    }

    if (connectResult != HTTP_CALLBACK_REASON_OK)
    {
        workloadCtx->status = EISErr_ConnErr;
    }
}

//
// EIS Communication Functions
//

/**
 * @brief Sends an EIS request to @p apiUriPath on @p udsSocketPath with content @p payload, times out after @p timeoutMS milliseconds
 * @details Caller must release @p responseBuffer with free()
 * @param udsSocketPath the path to the UDS socket on the machine
 * @param apiUriPath the API URI you are trying to send the request to which lives on @p udsSocketPath
 * @param payload an optional payload to be sent with the request to the @p apiUriPath , if NULL the request is a GET otherwise it is a POST
 * @param timeoutMS the timeoutMS for the request in milliseconds
 * @param responseBuffer the buffer that will be allocated by the function to hold the response
 * @returns Returns a value of EISErr
 */
EISErr SendEISRequest(
    const char* udsSocketPath,
    const char* apiUriPath,
    const char* payload,
    unsigned int timeoutMS,
    char** responseBuff)
{
    EISErr result = EISErr_Failed;

    if (udsSocketPath == NULL || apiUriPath == NULL || responseBuff == NULL)
    {
        return EISErr_InvalidArg;
    }

    *responseBuff = NULL;
    char* response = NULL;
    const int port = 80;
    size_t payloadLen = 0;

    HTTP_HEADERS_HANDLE httpHeadersHandle = NULL;

    HTTP_HEADERS_RESULT httpHeadersResult;

    HTTP_CLIENT_RESULT clientResult;
    HTTP_CLIENT_REQUEST_TYPE clientRequestType = HTTP_CLIENT_REQUEST_GET;

    SOCKETIO_CONFIG config = {};
    config.accepted_socket = NULL;
    config.hostname = udsSocketPath;
    config.port = port; // Check on this

    EIS_HTTP_WORKLOAD_CONTEXT workloadCtx;
    workloadCtx.continue_running = true;
    workloadCtx.http_response = NULL;
    workloadCtx.status = EISErr_Failed;

    HTTP_CLIENT_HANDLE clientHandle =
        uhttp_client_create(socketio_get_interface_description(), &config, on_eis_http_error, &workloadCtx);

    if (clientHandle == NULL)
    {
        goto done;
    }

    if (uhttp_client_set_option(clientHandle, OPTION_ADDRESS_TYPE, OPTION_ADDRESS_TYPE_DOMAIN_SOCKET)
        != HTTP_CLIENT_OK)
    {
        goto done;
    }

    if (uhttp_client_open(clientHandle, udsSocketPath, 0, on_eis_http_connected, &workloadCtx) != HTTP_CLIENT_OK)
    {
        result = workloadCtx.status;
        goto done;
    }

    if (payload != NULL)
    {
        httpHeadersHandle = HTTPHeaders_Alloc();
        if (httpHeadersHandle == NULL)
        {
            goto done;
        }

        httpHeadersResult = HTTPHeaders_AddHeaderNameValuePair(httpHeadersHandle, "Content-Type", "application/json");

        if (httpHeadersResult != HTTP_HEADERS_OK)
        {
            goto done;
        }

        clientRequestType = HTTP_CLIENT_REQUEST_POST;
        payloadLen = strlen(payload);
    }

    clientResult = uhttp_client_execute_request(
        clientHandle,
        clientRequestType,
        apiUriPath,
        httpHeadersHandle,
        (const unsigned char*)payload,
        payloadLen,
        on_eis_http_recv,
        &workloadCtx);

    if (clientResult != HTTP_CLIENT_OK)
    {
        goto done;
    }

    time_t startTime = get_time(NULL);
    bool timedOut = false;

    do
    {
        uhttp_client_dowork(clientHandle);
        timedOut = (difftime(get_time(NULL), startTime) > timeoutMS);

    } while (workloadCtx.continue_running == true && !timedOut);

    if (timedOut)
    {
        result = EISErr_TimeoutErr;
        goto done;
    }

    if (workloadCtx.status != EISErr_Ok)
    {
        result = workloadCtx.status;
        goto done;
    }

    size_t responseLen = 0;
    if (BUFFER_size(workloadCtx.http_response, &responseLen) != 0)
    {
        goto done;
    }

    if (responseLen > EIS_RESP_SIZE_MAX || responseLen < EIS_RESP_SIZE_MIN)
    {
        result = EISErr_RecvRespOutOfLimitsErr;
        goto done;
    }

    response = (char*)malloc(responseLen + 1);

    if (response == NULL)
    {
        goto done;
    }

    memcpy(response, BUFFER_u_char(workloadCtx.http_response), responseLen);
    response[responseLen] = '\0';

    result = EISErr_Ok;

done:

    // Cleanup

    HTTPHeaders_Free(httpHeadersHandle);
    uhttp_client_close(clientHandle, NULL, NULL);
    uhttp_client_destroy(clientHandle);

    if (workloadCtx.http_response != NULL)
    {
        BUFFER_delete(workloadCtx.http_response);
    }

    if (result != EISErr_Ok)
    {
        free(response);
        response = NULL;
    }

    *responseBuff = response;

    return result;
}

/**
 * @brief Requests the identities from the EIS /identity/ URI
 * @details The identity response returns the hub hostname, device id, and key handle
 * Caller must release @p responseBuffer with free()
 * @param timeoutMS max timeoutMS for the request in milliseconds
 * @param responseBuffer the buffer that will be allocated by the function to hold the response
 * @returns Returns a value of EISErr
 */
EISErr RequestIdentitiesFromEIS(unsigned int timeoutMS, char** responseBuffer)
{
    EISErr result =
        SendEISRequest(EIS_UDS_IDENTITY_SOCKET_PATH, EIS_IDENTITY_REQUEST_URI, NULL, timeoutMS, responseBuffer);

    if (result != EISErr_Ok)
    {
        goto done;
    }

done:

    return result;
}

/**
 * @brief Requests the signed form of @p deviceUri and @p expiry using @p keyHandle with a request timeoutMS of @p timeoutMS
 * @details Caller should de-allocate @p responseBuffer using free()
 * @param keyHandle the handle for the key to be used for signing @p deviceUri and @p expiry
 * @param uri the uri to be used in the signature
 * @param expiry the expiration time in string format
 * @param timeoutMS the tiemout for the request in milliseconds
 * @returns A value of EISErr
 */
EISErr RequestSignatureFromEIS(
    const char* keyHandle, const char* uri, const char* expiry, unsigned int timeoutMS, char** responseBuffer)
{
    EISErr result = EISErr_Failed;

    if (responseBuffer == NULL || keyHandle == NULL || uri == NULL || expiry == NULL)
    {
        return EISErr_InvalidArg;
    }

    *responseBuffer = NULL;

    char* response = NULL;

    char* serializedPayload = NULL;

    char* uriToSign = NULL;
    STRING_HANDLE encodedUriToSign = NULL;

    JSON_Value* payloadValue = json_value_init_object();

    if (payloadValue == NULL)
    {
        goto done;
    }

    /**
     * Example KeyService Sign Request
     * {
     *  "keyHandle":"foo",
     *  "algorithm":"HMAC-256",
     *  "parameters" : {
     *      "message":"something-to-be-signed"
     *  }
     * }
     */
    JSON_Object* payloadObj = json_value_get_object(payloadValue);

    if (payloadObj == NULL)
    {
        goto done;
    }

    uriToSign = ADUC_StringFormat("%s\n%s", uri, expiry);

    if (uriToSign == NULL)
    {
        goto done;
    }

    encodedUriToSign = Azure_Base64_Encode_Bytes((unsigned char*)uriToSign, strlen(uriToSign));

    if (encodedUriToSign == NULL)
    {
        goto done;
    }

    if (json_object_set_string(payloadObj, EIS_SIGN_REQ_KEYHANDLE_FIELD, keyHandle) != JSONSuccess)
    {
        goto done;
    }

    if (json_object_set_string(payloadObj, EIS_SIGN_REQ_ALG_FIELD, EIS_SIGN_ALGORITHM) != JSONSuccess)
    {
        goto done;
    }

    if (json_object_dotset_string(payloadObj, EIS_SIGN_REQ_DOTSET_PARAMS_MSG_FIELD, STRING_c_str(encodedUriToSign))
        != JSONSuccess)
    {
        goto done;
    }

    serializedPayload = json_serialize_to_string(payloadValue);

    if (serializedPayload == NULL)
    {
        goto done;
    }

    result = SendEISRequest(EIS_UDS_SIGN_SOCKET_PATH, EIS_SIGN_REQUEST_URI, serializedPayload, timeoutMS, &response);

    if (result != EISErr_Ok)
    {
        goto done;
    }

    result = EISErr_Ok;
done:

    json_value_free(payloadValue);

    free(serializedPayload);

    STRING_delete(encodedUriToSign);

    free(uriToSign);

    if (result != EISErr_Ok)
    {
        free(response);
        response = NULL;
    }

    *responseBuffer = response;

    return result;
}

/**
 * @brief Requests the signature related to @p certId from EIS
 * @details Caller should de-allocate @p responseBuffer using free()
 * @param[in] certId the identifier associated with the certificate being retrieved
 * @param[in] timeoutMS the timeout for the call
 * @param[out] responseBuffer ptr to the buffer which will hold the response from EIS
 * @returns a value of EISErr
 */
EISErr RequestCertificateFromEIS(const char* certId, unsigned int timeoutMS, char** responseBuffer)
{
    EISErr result = EISErr_Failed;

    char* requestURI = NULL;

    if (responseBuffer == NULL)
    {
        result = EISErr_InvalidArg;
        goto done;
    }

    requestURI = ADUC_StringFormat("%s/%s?%s", EIS_CERT_URI, certId, EIS_API_VERSION);

    if (requestURI == NULL)
    {
        goto done;
    }

    result = SendEISRequest(EIS_UDS_CERT_SOCKET_PATH, requestURI, NULL, timeoutMS, responseBuffer);

    if (result != EISErr_Ok)
    {
        goto done;
    }

done:

    free(requestURI);

    return result;
}
