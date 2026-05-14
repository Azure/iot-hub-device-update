/**
 * @file adu_direct_comm.c
 * @brief ADU Direct Communication Provider — v4 RPC-over-HTTP protocol.
 *
 * Implements the ADUC_CommunicationVtable using the ADU Device Data Plane
 * Protocol v3 with three RPC operations:
 *   POST /syncConfiguration
 *   POST /requestUpdates
 *   POST /reportStatus
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "adu_direct_comm.h"

#include "aduc/communication_vtable.h"
#include "aduc/extension_context.h"
#include "aduc/extension_descriptor.h"
#include "aduc/extension_types.h"

#include <curl/curl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define strncasecmp _strnicmp
#else
#include <fcntl.h>
#include <strings.h>
#include <unistd.h>
#endif

/* -------------------------------------------------------------------------- */
/*                              Constants                                      */
/* -------------------------------------------------------------------------- */

#define ADU_API_VERSION "2026-11-02-preview"
#define ADU_USER_AGENT  "adu-agent/2.0.0"
#define ADU_SDK_VERSION "2.0.0"
#define ADU_AGENT_PROFILE 3

#define MAX_CACHED_FILE_URLS 32
#define MAX_URL_LEN 2048
#define MAX_FILE_ID_LEN 64

/* -------------------------------------------------------------------------- */
/*                            Module-level state                               */
/* -------------------------------------------------------------------------- */

static struct
{
    CURL* curl;
    char endpoint[512];
    char deviceId[128];
    char certPath[512];
    char keyPath[512];
    char manufacturer[128];
    char model[128];
    char installedUpdateId[256];
    uint32_t pollIntervalSec;
    ADUC_CommConnectionState connState;

    /* v3 protocol cached state */
    char agentInfoETag[128];
    char serviceConfigETag[128];
    char rootKeyDownloadUrl[MAX_URL_LEN];

    /* Retry-After state */
    time_t retryAfterUntil;

    /* Whether first poll has been issued (for cold-start jitter) */
    bool firstPollDone;
} s_state;

/* Cached file URL map from last requestUpdates response */
static struct
{
    char fileId[MAX_FILE_ID_LEN];
    char url[MAX_URL_LEN];
} s_fileUrls[MAX_CACHED_FILE_URLS];

static int s_fileUrlCount;

/* -------------------------------------------------------------------------- */
/*                         Curl response buffer helper                         */
/* -------------------------------------------------------------------------- */

typedef struct
{
    char* data;
    size_t size;
} ResponseBuffer;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t totalSize = size * nmemb;
    ResponseBuffer* buf = (ResponseBuffer*)userp;

    char* tmp = realloc(buf->data, buf->size + totalSize + 1);
    if (tmp == NULL)
    {
        return 0; /* out of memory */
    }

    buf->data = tmp;
    memcpy(&(buf->data[buf->size]), contents, totalSize);
    buf->size += totalSize;
    buf->data[buf->size] = '\0';

    return totalSize;
}

/* -------------------------------------------------------------------------- */
/*                     Header callback for Retry-After                         */
/* -------------------------------------------------------------------------- */

static long s_retryAfterSeconds;

static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata)
{
    (void)userdata;
    size_t totalSize = size * nitems;

    if (totalSize > 13 && strncasecmp(buffer, "Retry-After:", 12) == 0)
    {
        const char* val = buffer + 12;
        while (*val == ' ')
        {
            val++;
        }
        s_retryAfterSeconds = strtol(val, NULL, 10);
    }

    return totalSize;
}

/* -------------------------------------------------------------------------- */
/*                         UUID generation helper                              */
/* -------------------------------------------------------------------------- */

static void GenerateUuid(char* buf, size_t bufLen)
{
    if (bufLen < 37)
    {
        buf[0] = '\0';
        return;
    }

#ifndef _WIN32
    /* Try Linux kernel UUID generator first */
    int fd = open("/proc/sys/kernel/random/uuid", O_RDONLY);
    if (fd >= 0)
    {
        ssize_t n = read(fd, buf, 36);
        close(fd);
        if (n == 36)
        {
            buf[36] = '\0';
            return;
        }
    }
#endif

    /* Fallback: generate from random bytes */
    unsigned char bytes[16];
#ifdef _WIN32
    /* Use CryptGenRandom or RtlGenRandom is not available, use rand as fallback */
    for (int i = 0; i < 16; i++)
    {
        bytes[i] = (unsigned char)(rand() & 0xFF);
    }
#else
    int ufd = open("/dev/urandom", O_RDONLY);
    if (ufd >= 0)
    {
        read(ufd, bytes, sizeof(bytes));
        close(ufd);
    }
    else
    {
        for (int i = 0; i < 16; i++)
        {
            bytes[i] = (unsigned char)(rand() & 0xFF);
        }
    }
#endif

    /* Set version 4 and variant bits */
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    snprintf(
        buf, bufLen,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        bytes[0], bytes[1], bytes[2], bytes[3],
        bytes[4], bytes[5],
        bytes[6], bytes[7],
        bytes[8], bytes[9],
        bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
}

/* -------------------------------------------------------------------------- */
/*                      Minimal JSON string helpers                            */
/* -------------------------------------------------------------------------- */

/**
 * @brief Extract a JSON string value for a given key (flat or one level deep).
 *
 * Handles escaped quotes within values. Writes result into @p out.
 * @return true if found, false otherwise.
 */
static bool JsonExtractString(const char* json, const char* key, char* out, size_t outLen)
{
    if (json == NULL || key == NULL || out == NULL || outLen == 0)
    {
        return false;
    }

    /* Build search pattern: "key":" or "key" : " */
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char* pos = strstr(json, pattern);
    if (pos == NULL)
    {
        return false;
    }

    pos += strlen(pattern);

    /* Skip whitespace and colon */
    while (*pos == ' ' || *pos == ':' || *pos == '\t' || *pos == '\n' || *pos == '\r')
    {
        pos++;
    }

    if (*pos == '"')
    {
        pos++; /* skip opening quote */
        size_t i = 0;
        while (*pos != '\0' && i < outLen - 1)
        {
            if (*pos == '\\' && *(pos + 1) != '\0')
            {
                pos++;
                out[i++] = *pos++;
            }
            else if (*pos == '"')
            {
                break;
            }
            else
            {
                out[i++] = *pos++;
            }
        }
        out[i] = '\0';
        return true;
    }

    /* Handle null literal */
    if (strncmp(pos, "null", 4) == 0)
    {
        out[0] = '\0';
        return true;
    }

    return false;
}

/**
 * @brief Extract the "error.code" value from a JSON error response.
 */
static bool JsonExtractErrorCode(const char* json, char* out, size_t outLen)
{
    if (json == NULL)
    {
        return false;
    }

    const char* errObj = strstr(json, "\"error\"");
    if (errObj == NULL)
    {
        return false;
    }

    const char* brace = strchr(errObj, '{');
    if (brace == NULL)
    {
        return false;
    }

    return JsonExtractString(brace, "code", out, outLen);
}

/**
 * @brief Extract a JSON object substring (first level) for a given key.
 *
 * Returns pointer into the original json and sets length. Handles nested braces.
 */
static const char* JsonFindObject(const char* json, const char* key, size_t* outLen)
{
    if (json == NULL || key == NULL)
    {
        return NULL;
    }

    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char* pos = strstr(json, pattern);
    if (pos == NULL)
    {
        return NULL;
    }

    pos += strlen(pattern);
    while (*pos == ' ' || *pos == ':' || *pos == '\t' || *pos == '\n' || *pos == '\r')
    {
        pos++;
    }

    if (*pos != '{')
    {
        return NULL;
    }

    int depth = 0;
    const char* start = pos;
    while (*pos != '\0')
    {
        if (*pos == '{')
        {
            depth++;
        }
        else if (*pos == '}')
        {
            depth--;
            if (depth == 0)
            {
                if (outLen != NULL)
                {
                    *outLen = (size_t)(pos - start + 1);
                }
                return start;
            }
        }
        else if (*pos == '"')
        {
            /* Skip string contents */
            pos++;
            while (*pos != '\0' && *pos != '"')
            {
                if (*pos == '\\')
                {
                    pos++;
                }
                pos++;
            }
        }
        pos++;
    }

    return NULL;
}

/**
 * @brief Parse the "fileUrls" object from a requestUpdates response into the cache.
 */
static void ParseFileUrls(const char* json)
{
    s_fileUrlCount = 0;

    size_t objLen = 0;
    const char* fileUrlsObj = JsonFindObject(json, "fileUrls", &objLen);
    if (fileUrlsObj == NULL)
    {
        return;
    }

    /* Parse key-value pairs within the fileUrls object */
    const char* pos = fileUrlsObj + 1; /* skip opening brace */
    const char* end = fileUrlsObj + objLen - 1; /* before closing brace */

    while (pos < end && s_fileUrlCount < MAX_CACHED_FILE_URLS)
    {
        /* Find next key */
        const char* keyStart = strchr(pos, '"');
        if (keyStart == NULL || keyStart >= end)
        {
            break;
        }
        keyStart++;

        const char* keyEnd = strchr(keyStart, '"');
        if (keyEnd == NULL || keyEnd >= end)
        {
            break;
        }

        size_t keyLen = (size_t)(keyEnd - keyStart);
        if (keyLen >= MAX_FILE_ID_LEN)
        {
            keyLen = MAX_FILE_ID_LEN - 1;
        }

        /* Find value */
        pos = keyEnd + 1;
        const char* valStart = strchr(pos, '"');
        if (valStart == NULL || valStart >= end)
        {
            break;
        }
        valStart++;

        /* Find closing quote for value, handling escapes */
        const char* valEnd = valStart;
        while (valEnd < end)
        {
            if (*valEnd == '\\')
            {
                valEnd += 2;
                continue;
            }
            if (*valEnd == '"')
            {
                break;
            }
            valEnd++;
        }

        size_t valLen = (size_t)(valEnd - valStart);
        if (valLen >= MAX_URL_LEN)
        {
            valLen = MAX_URL_LEN - 1;
        }

        memcpy(s_fileUrls[s_fileUrlCount].fileId, keyStart, keyLen);
        s_fileUrls[s_fileUrlCount].fileId[keyLen] = '\0';
        memcpy(s_fileUrls[s_fileUrlCount].url, valStart, valLen);
        s_fileUrls[s_fileUrlCount].url[valLen] = '\0';
        s_fileUrlCount++;

        pos = valEnd + 1;
    }
}

/* -------------------------------------------------------------------------- */
/*                          Helper: perform HTTP POST                          */
/* -------------------------------------------------------------------------- */

typedef struct
{
    long httpCode;
    long retryAfterSec;
} HttpResult;

/**
 * @brief Perform an HTTP POST request with v3 protocol headers and mTLS.
 *
 * Sets Content-Type, User-Agent, x-ms-device-id, x-ms-correlation-id.
 * Configures mTLS if cert/key paths are available.
 * Enforces HTTPS.
 */
static ADUC_Result2 PerformRequest(
    const char* url,
    const char* body,
    long timeoutMs,
    ResponseBuffer* response,
    HttpResult* httpResult)
{
    ADUC_Result2 result = ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_IO, 1);

    if (httpResult != NULL)
    {
        httpResult->httpCode = 0;
        httpResult->retryAfterSec = 0;
    }

    if (s_state.curl == NULL)
    {
        return result;
    }

    /* Enforce HTTPS */
    if (strncmp(url, "https://", 8) != 0)
    {
        fprintf(stderr, "[adu-direct] Rejecting non-HTTPS URL: %s\n", url);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_PROTOCOL, 1);
    }

    curl_easy_reset(s_state.curl);
    curl_easy_setopt(s_state.curl, CURLOPT_URL, url);
    curl_easy_setopt(s_state.curl, CURLOPT_TIMEOUT_MS, timeoutMs > 0 ? timeoutMs : 30000L);
    curl_easy_setopt(s_state.curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(s_state.curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(s_state.curl, CURLOPT_POST, 1L);

    if (body != NULL)
    {
        curl_easy_setopt(s_state.curl, CURLOPT_POSTFIELDS, body);
    }
    else
    {
        curl_easy_setopt(s_state.curl, CURLOPT_POSTFIELDS, "");
    }

    /* Retry-After header extraction */
    s_retryAfterSeconds = 0;
    curl_easy_setopt(s_state.curl, CURLOPT_HEADERFUNCTION, HeaderCallback);

    /* mTLS configuration */
    if (s_state.certPath[0] != '\0')
    {
        curl_easy_setopt(s_state.curl, CURLOPT_SSLCERT, s_state.certPath);
    }
    if (s_state.keyPath[0] != '\0')
    {
        curl_easy_setopt(s_state.curl, CURLOPT_SSLKEY, s_state.keyPath);
    }
    curl_easy_setopt(s_state.curl, CURLOPT_SSL_VERIFYPEER, 1L);

    /* Build headers */
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    char uaHeader[128];
    snprintf(uaHeader, sizeof(uaHeader), "User-Agent: %s", ADU_USER_AGENT);
    headers = curl_slist_append(headers, uaHeader);

    if (s_state.deviceId[0] != '\0')
    {
        char deviceHeader[256];
        snprintf(deviceHeader, sizeof(deviceHeader), "x-ms-device-id: %s", s_state.deviceId);
        headers = curl_slist_append(headers, deviceHeader);
    }

    char correlationId[64];
    GenerateUuid(correlationId, sizeof(correlationId));
    char corrHeader[128];
    snprintf(corrHeader, sizeof(corrHeader), "x-ms-correlation-id: %s", correlationId);
    headers = curl_slist_append(headers, corrHeader);

    curl_easy_setopt(s_state.curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(s_state.curl);
    if (res == CURLE_OK)
    {
        long httpCode = 0;
        curl_easy_getinfo(s_state.curl, CURLINFO_RESPONSE_CODE, &httpCode);

        if (httpResult != NULL)
        {
            httpResult->httpCode = httpCode;
            httpResult->retryAfterSec = s_retryAfterSeconds;
        }

        if (httpCode >= 200 && httpCode < 300)
        {
            result = ADUC_RESULT2_SUCCESS;
        }
        else if (httpCode == 429 || httpCode == 503)
        {
            /* Rate-limited or service unavailable — record Retry-After */
            long retrySec = s_retryAfterSeconds > 0 ? s_retryAfterSeconds : 60;
            s_state.retryAfterUntil = time(NULL) + retrySec;
            result = ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_IO, (uint16_t)httpCode);
        }
        else
        {
            result = ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_PROTOCOL, (uint16_t)(httpCode & 0xFFFF));
        }
    }

    curl_slist_free_all(headers);
    return result;
}

/* -------------------------------------------------------------------------- */
/*                     v3 RPC: syncConfiguration                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Call the syncConfiguration RPC endpoint.
 *
 * @param sendFullAgentInfo If true, include full agentInfo block. Otherwise minimal request.
 * @return ADUC_Result2 indicating success or failure.
 */
static ADUC_Result2 SyncConfiguration(bool sendFullAgentInfo)
{
    char url[1024];
    snprintf(
        url, sizeof(url),
        "%s/syncConfiguration?api-version=%s",
        s_state.endpoint, ADU_API_VERSION);

    char body[2048];
    if (sendFullAgentInfo)
    {
        snprintf(
            body, sizeof(body),
            "{"
            "\"agentInfoETag\":\"%s\","
            "\"agentInfo\":{"
            "\"agentSdkVersion\":\"%s\","
            "\"agentProfile\":%d,"
            "\"compatibilityProperties\":{\"manufacturer\":\"%s\",\"model\":\"%s\"}"
            "}"
            "}",
            s_state.agentInfoETag,
            ADU_SDK_VERSION,
            ADU_AGENT_PROFILE,
            s_state.manufacturer,
            s_state.model);
    }
    else
    {
        /* Minimal request — just send the ETag, no agentInfo */
        snprintf(
            body, sizeof(body),
            "{\"agentInfoETag\":\"%s\"}",
            s_state.agentInfoETag);
    }

    ResponseBuffer response = { NULL, 0 };
    HttpResult httpResult = { 0, 0 };
    ADUC_Result2 result = PerformRequest(url, body, 30000L, &response, &httpResult);

    if (ADUC_RESULT2_IS_SUCCESS(result) && response.data != NULL)
    {
        /* Parse serviceConfigETag */
        JsonExtractString(response.data, "serviceConfigETag", s_state.serviceConfigETag, sizeof(s_state.serviceConfigETag));

        /* Parse rootKeyDownloadUrl from serviceConfiguration object */
        size_t scLen = 0;
        const char* sc = JsonFindObject(response.data, "serviceConfiguration", &scLen);
        if (sc != NULL)
        {
            /* Extract from the sub-object */
            char scBuf[2048];
            size_t copyLen = scLen < sizeof(scBuf) - 1 ? scLen : sizeof(scBuf) - 1;
            memcpy(scBuf, sc, copyLen);
            scBuf[copyLen] = '\0';
            JsonExtractString(scBuf, "rootKeyDownloadUrl", s_state.rootKeyDownloadUrl, sizeof(s_state.rootKeyDownloadUrl));
        }
    }
    else if (response.data != NULL)
    {
        /* Log error details from non-success responses */
        char errorCode[128] = { 0 };
        if (JsonExtractErrorCode(response.data, errorCode, sizeof(errorCode)))
        {
            fprintf(stderr, "[adu-direct] syncConfiguration error: %s\n", errorCode);
        }
    }

    free(response.data);
    return result;
}

/* -------------------------------------------------------------------------- */
/*                     v3 error handling helper                                */
/* -------------------------------------------------------------------------- */

typedef enum
{
    ErrorAction_None,
    ErrorAction_ResyncFull,
    ErrorAction_ResyncMinimal,
    ErrorAction_Retry,
    ErrorAction_Fatal,
} ErrorAction;

static ErrorAction ClassifyError(const char* responseBody, long httpCode)
{
    if (httpCode == 429 || httpCode == 503)
    {
        return ErrorAction_Retry;
    }

    char errorCode[128] = { 0 };
    if (responseBody == NULL || !JsonExtractErrorCode(responseBody, errorCode, sizeof(errorCode)))
    {
        return ErrorAction_None;
    }

    if (strcmp(errorCode, "UNKNOWN_AGENT_INFO_VERSION") == 0)
    {
        return ErrorAction_ResyncFull;
    }
    if (strcmp(errorCode, "OUTDATED_SERVICE_CONFIG") == 0)
    {
        return ErrorAction_ResyncMinimal;
    }
    if (strcmp(errorCode, "UNSUPPORTED_API_VERSION") == 0)
    {
        return ErrorAction_Retry;
    }

    /* Fatal errors — log and don't retry */
    if (strcmp(errorCode, "UNSUPPORTED_AGENT_PROFILE") == 0 ||
        strcmp(errorCode, "INVALID_COMPATIBILITY_PROPERTIES") == 0 ||
        strcmp(errorCode, "MAX_DEVICE_CLASSES_EXCEEDED") == 0 ||
        strcmp(errorCode, "UNSUPPORTED_INSTALLED_VERSION") == 0 ||
        strcmp(errorCode, "UPDATE_ACCOUNT_NOT_LINKED") == 0)
    {
        fprintf(stderr, "[adu-direct] Fatal service error: %s\n", errorCode);
        return ErrorAction_Fatal;
    }

    return ErrorAction_None;
}

/* -------------------------------------------------------------------------- */
/*                       Extension lifecycle functions                         */
/* -------------------------------------------------------------------------- */

ADUC_Result2 AduDirect_Initialize(const ADUC_ExtensionContext* context)
{
    (void)context;

    memset(&s_state, 0, sizeof(s_state));
    memset(s_fileUrls, 0, sizeof(s_fileUrls));
    s_fileUrlCount = 0;
    s_state.connState = ADUC_COMM_STATE_DISCONNECTED;

    srand((unsigned int)time(NULL));
    curl_global_init(CURL_GLOBAL_DEFAULT);
    return ADUC_RESULT2_SUCCESS;
}

void AduDirect_Uninitialize(void)
{
    if (s_state.curl != NULL)
    {
        curl_easy_cleanup(s_state.curl);
        s_state.curl = NULL;
    }
    curl_global_cleanup();
    s_state.connState = ADUC_COMM_STATE_DISCONNECTED;
}

/* -------------------------------------------------------------------------- */
/*                     Communication vtable implementations                   */
/* -------------------------------------------------------------------------- */

ADUC_Result2 AduDirect_Connect(const ADUC_CommConfig* config)
{
    ADUC_Result2 result = ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_IO, 1);

    if (config == NULL || config->endpoint == NULL || config->deviceId == NULL)
    {
        return result;
    }

    s_state.curl = curl_easy_init();
    if (s_state.curl == NULL)
    {
        return result;
    }

    snprintf(s_state.endpoint, sizeof(s_state.endpoint), "%s", config->endpoint);
    snprintf(s_state.deviceId, sizeof(s_state.deviceId), "%s", config->deviceId);

    if (config->manufacturer != NULL)
    {
        snprintf(s_state.manufacturer, sizeof(s_state.manufacturer), "%s", config->manufacturer);
    }
    else
    {
        snprintf(s_state.manufacturer, sizeof(s_state.manufacturer), "unknown");
    }
    if (config->model != NULL)
    {
        snprintf(s_state.model, sizeof(s_state.model), "%s", config->model);
    }
    else
    {
        snprintf(s_state.model, sizeof(s_state.model), "unknown");
    }
    if (config->installedUpdateId != NULL)
    {
        snprintf(s_state.installedUpdateId, sizeof(s_state.installedUpdateId), "%s", config->installedUpdateId);
    }
    else
    {
        s_state.installedUpdateId[0] = '\0';
    }

    if (config->certPath != NULL)
    {
        snprintf(s_state.certPath, sizeof(s_state.certPath), "%s", config->certPath);
    }
    if (config->keyPath != NULL)
    {
        snprintf(s_state.keyPath, sizeof(s_state.keyPath), "%s", config->keyPath);
    }

    s_state.pollIntervalSec = config->pollIntervalSec > 0 ? config->pollIntervalSec : 60;

    /* Generate initial agentInfoETag */
    snprintf(s_state.agentInfoETag, sizeof(s_state.agentInfoETag), "v3");

    s_state.firstPollDone = false;

    /* Perform initial syncConfiguration with full agentInfo */
    result = SyncConfiguration(true);

    if (ADUC_RESULT2_IS_SUCCESS(result))
    {
        s_state.connState = ADUC_COMM_STATE_CONNECTED;
    }
    else
    {
        /* Connection failed — clean up curl handle */
        curl_easy_cleanup(s_state.curl);
        s_state.curl = NULL;
        s_state.connState = ADUC_COMM_STATE_DISCONNECTED;
    }

    return result;
}

void AduDirect_Disconnect(void)
{
    if (s_state.curl != NULL)
    {
        curl_easy_cleanup(s_state.curl);
        s_state.curl = NULL;
    }

    memset(s_state.endpoint, 0, sizeof(s_state.endpoint));
    memset(s_state.deviceId, 0, sizeof(s_state.deviceId));
    memset(s_state.certPath, 0, sizeof(s_state.certPath));
    memset(s_state.keyPath, 0, sizeof(s_state.keyPath));
    memset(s_state.manufacturer, 0, sizeof(s_state.manufacturer));
    memset(s_state.model, 0, sizeof(s_state.model));
    memset(s_state.installedUpdateId, 0, sizeof(s_state.installedUpdateId));
    memset(s_state.agentInfoETag, 0, sizeof(s_state.agentInfoETag));
    memset(s_state.serviceConfigETag, 0, sizeof(s_state.serviceConfigETag));
    memset(s_state.rootKeyDownloadUrl, 0, sizeof(s_state.rootKeyDownloadUrl));
    memset(s_fileUrls, 0, sizeof(s_fileUrls));
    s_fileUrlCount = 0;
    s_state.connState = ADUC_COMM_STATE_DISCONNECTED;
}

ADUC_CommConnectionState AduDirect_GetConnectionState(void)
{
    return s_state.connState;
}

ADUC_Result2 AduDirect_Poll(ADUC_CommMessage* outMsg, uint32_t timeoutMs)
{
    ADUC_Result2 result = ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_IO, 1);

    if (outMsg == NULL || s_state.connState != ADUC_COMM_STATE_CONNECTED)
    {
        return result;
    }

    /* Respect Retry-After backoff */
    time_t now = time(NULL);
    if (s_state.retryAfterUntil > now)
    {
        result = ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_STATE, 1);
        return result;
    }

    /* Cold-start jitter: random delay before first poll */
    if (!s_state.firstPollDone && s_state.pollIntervalSec > 0)
    {
        unsigned int jitterSec = (unsigned int)(rand() % (int)s_state.pollIntervalSec);
        if (jitterSec > 0)
        {
#ifdef _WIN32
            Sleep(jitterSec * 1000);
#else
            sleep(jitterSec);
#endif
        }
        s_state.firstPollDone = true;
    }

    /* Build requestUpdates body */
    char installedIdField[512];
    if (s_state.installedUpdateId[0] != '\0')
    {
        snprintf(installedIdField, sizeof(installedIdField), "\"%s\"", s_state.installedUpdateId);
    }
    else
    {
        snprintf(installedIdField, sizeof(installedIdField), "null");
    }

    char body[1024];
    snprintf(
        body, sizeof(body),
        "{"
        "\"agentInfoETag\":\"%s\","
        "\"serviceConfigETag\":\"%s\","
        "\"installedUpdateId\":%s"
        "}",
        s_state.agentInfoETag,
        s_state.serviceConfigETag,
        installedIdField);

    char url[1024];
    snprintf(
        url, sizeof(url),
        "%s/requestUpdates?api-version=%s",
        s_state.endpoint, ADU_API_VERSION);

    ResponseBuffer response = { NULL, 0 };
    HttpResult httpResult = { 0, 0 };
    result = PerformRequest(url, body, (long)timeoutMs, &response, &httpResult);

    if (ADUC_RESULT2_IS_SUCCESS(result))
    {
        if (response.data != NULL && response.size > 0)
        {
            /* Cache fileUrls from the update object */
            size_t updateLen = 0;
            const char* updateObj = JsonFindObject(response.data, "updateMetadata", &updateLen);
            if (updateObj != NULL)
            {
                char updateBuf[8192];
                size_t copyLen = updateLen < sizeof(updateBuf) - 1 ? updateLen : sizeof(updateBuf) - 1;
                memcpy(updateBuf, updateObj, copyLen);
                updateBuf[copyLen] = '\0';
                ParseFileUrls(updateBuf);
            }

            outMsg->type = ADUC_COMM_EVENT_DEPLOYMENT_AVAILABLE;
            outMsg->payload = response.data; /* Transfer ownership to caller */
            outMsg->payloadLen = response.size;
            return ADUC_RESULT2_SUCCESS;
        }
        else
        {
            /* HTTP 200 with empty body — no update available */
            free(response.data);
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_STATE, 1);
        }
    }

    /* Handle v3 error codes with retry logic */
    ErrorAction action = ClassifyError(response.data, httpResult.httpCode);
    free(response.data);

    if (action == ErrorAction_ResyncFull)
    {
        /* UNKNOWN_AGENT_INFO_VERSION — re-sync with full agentInfo, then retry */
        ADUC_Result2 syncResult = SyncConfiguration(true);
        if (ADUC_RESULT2_IS_SUCCESS(syncResult))
        {
            /* Retry requestUpdates (non-recursive — one retry only) */
            snprintf(
                body, sizeof(body),
                "{"
                "\"agentInfoETag\":\"%s\","
                "\"serviceConfigETag\":\"%s\","
                "\"installedUpdateId\":%s"
                "}",
                s_state.agentInfoETag,
                s_state.serviceConfigETag,
                installedIdField);

            ResponseBuffer retryResp = { NULL, 0 };
            HttpResult retryHttp = { 0, 0 };
            result = PerformRequest(url, body, (long)timeoutMs, &retryResp, &retryHttp);

            if (ADUC_RESULT2_IS_SUCCESS(result) && retryResp.data != NULL && retryResp.size > 0)
            {
                size_t updateLen = 0;
                const char* updateObj = JsonFindObject(retryResp.data, "updateMetadata", &updateLen);
                if (updateObj != NULL)
                {
                    char updateBuf[8192];
                    size_t copyLen = updateLen < sizeof(updateBuf) - 1 ? updateLen : sizeof(updateBuf) - 1;
                    memcpy(updateBuf, updateObj, copyLen);
                    updateBuf[copyLen] = '\0';
                    ParseFileUrls(updateBuf);
                }

                outMsg->type = ADUC_COMM_EVENT_DEPLOYMENT_AVAILABLE;
                outMsg->payload = retryResp.data;
                outMsg->payloadLen = retryResp.size;
                return ADUC_RESULT2_SUCCESS;
            }

            if (ADUC_RESULT2_IS_SUCCESS(result) && (retryResp.data == NULL || retryResp.size == 0))
            {
                free(retryResp.data);
                return ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_STATE, 1);
            }

            free(retryResp.data);
        }
    }
    else if (action == ErrorAction_ResyncMinimal)
    {
        /* OUTDATED_SERVICE_CONFIG — re-sync minimal, then retry */
        ADUC_Result2 syncResult = SyncConfiguration(false);
        if (ADUC_RESULT2_IS_SUCCESS(syncResult))
        {
            snprintf(
                body, sizeof(body),
                "{"
                "\"agentInfoETag\":\"%s\","
                "\"serviceConfigETag\":\"%s\","
                "\"installedUpdateId\":%s"
                "}",
                s_state.agentInfoETag,
                s_state.serviceConfigETag,
                installedIdField);

            ResponseBuffer retryResp = { NULL, 0 };
            HttpResult retryHttp = { 0, 0 };
            result = PerformRequest(url, body, (long)timeoutMs, &retryResp, &retryHttp);

            if (ADUC_RESULT2_IS_SUCCESS(result) && retryResp.data != NULL && retryResp.size > 0)
            {
                size_t updateLen = 0;
                const char* updateObj = JsonFindObject(retryResp.data, "updateMetadata", &updateLen);
                if (updateObj != NULL)
                {
                    char updateBuf[8192];
                    size_t copyLen = updateLen < sizeof(updateBuf) - 1 ? updateLen : sizeof(updateBuf) - 1;
                    memcpy(updateBuf, updateObj, copyLen);
                    updateBuf[copyLen] = '\0';
                    ParseFileUrls(updateBuf);
                }

                outMsg->type = ADUC_COMM_EVENT_DEPLOYMENT_AVAILABLE;
                outMsg->payload = retryResp.data;
                outMsg->payloadLen = retryResp.size;
                return ADUC_RESULT2_SUCCESS;
            }

            if (ADUC_RESULT2_IS_SUCCESS(result) && (retryResp.data == NULL || retryResp.size == 0))
            {
                free(retryResp.data);
                return ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_STATE, 1);
            }

            free(retryResp.data);
        }
    }

    return result;
}

ADUC_Result2 AduDirect_ReportState(const ADUC_AgentState* state)
{
    (void)state;

    /* v3 protocol does not have a separate ReportState RPC.
     * Agent state is communicated via syncConfiguration and requestUpdates. */
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 AduDirect_ReportResult(const ADUC_DeploymentResult2* deployResult)
{
    ADUC_Result2 result = ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_IO, 1);

    if (deployResult == NULL || deployResult->workflowId == NULL ||
        s_state.connState != ADUC_COMM_STATE_CONNECTED)
    {
        return result;
    }

    /* Respect Retry-After backoff */
    time_t now = time(NULL);
    if (s_state.retryAfterUntil > now)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_IO, 429);
    }

    char url[1024];
    snprintf(
        url, sizeof(url),
        "%s/reportStatus?api-version=%s",
        s_state.endpoint, ADU_API_VERSION);

    /* Build the reportStatus JSON body */
    /* v4: FAILED + NOT_APPLICABLE is prohibited — auto-correct to OTHER */
    ADUC_FailureOrigin effectiveOrigin = deployResult->failureOrigin;
    if (deployResult->outcome == ADUC_Outcome_Failed &&
        effectiveOrigin == ADUC_FailureOrigin_NotApplicable)
    {
        effectiveOrigin = ADUC_FailureOrigin_Other;
    }
    const char* outcomeStr = ADUC_Outcome_ToString(deployResult->outcome);
    const char* failureOriginStr = ADUC_FailureOrigin_ToString(effectiveOrigin);
    const char* extendedResultCodes = deployResult->extendedResultCodes != NULL
        ? deployResult->extendedResultCodes : "00000000";
    const char* resultDetails = deployResult->resultDetails != NULL
        ? deployResult->resultDetails : "";

    /* Format installedUpdateId — null or quoted string */
    char installedUpdateIdField[512];
    if (deployResult->installedUpdateId != NULL && deployResult->installedUpdateId[0] != '\0')
    {
        snprintf(installedUpdateIdField, sizeof(installedUpdateIdField),
                 "\"%s\"", deployResult->installedUpdateId);
    }
    else
    {
        snprintf(installedUpdateIdField, sizeof(installedUpdateIdField), "null");
    }

    /* Build stepResults portion if available */
    char stepResultsField[4096];
    if (deployResult->stepResultsJson != NULL && deployResult->stepResultsJson[0] != '\0')
    {
        snprintf(stepResultsField, sizeof(stepResultsField),
                 ",\"stepResults\":%s", deployResult->stepResultsJson);
    }
    else
    {
        stepResultsField[0] = '\0';
    }

    char body[8192];
    snprintf(
        body, sizeof(body),
        "{"
        "\"workflowId\":\"%s\","
        "\"installedUpdateId\":%s,"
        "\"lastInstallResult\":{"
        "\"outcome\":\"%s\","
        "\"failureOrigin\":\"%s\","
        "\"resultCode\":%" PRId64 ","
        "\"extendedResultCodes\":\"%s\","
        "\"resultDetails\":\"%s\""
        "%s"
        "}"
        "}",
        deployResult->workflowId,
        installedUpdateIdField,
        outcomeStr,
        failureOriginStr,
        deployResult->resultCode,
        extendedResultCodes,
        resultDetails,
        stepResultsField);

    ResponseBuffer response = { NULL, 0 };
    HttpResult httpResult = { 0, 0 };
    result = PerformRequest(url, body, 30000L, &response, &httpResult);

    if (!ADUC_RESULT2_IS_SUCCESS(result) && response.data != NULL)
    {
        char errorCode[128] = { 0 };
        if (JsonExtractErrorCode(response.data, errorCode, sizeof(errorCode)))
        {
            fprintf(stderr, "[adu-direct] reportStatus error: %s\n", errorCode);
        }
    }

    free(response.data);
    return result;
}

ADUC_Result2 AduDirect_GetDownloadUrl(const char* fileId, char* urlBuf, size_t urlBufLen)
{
    ADUC_Result2 result = ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_IO, 1);

    if (fileId == NULL || urlBuf == NULL || urlBufLen == 0)
    {
        return result;
    }

    /* Look up fileId in cached fileUrls map — no HTTP call needed */
    for (int i = 0; i < s_fileUrlCount; i++)
    {
        if (strcmp(s_fileUrls[i].fileId, fileId) == 0)
        {
            snprintf(urlBuf, urlBufLen, "%s", s_fileUrls[i].url);
            return ADUC_RESULT2_SUCCESS;
        }
    }

    /* fileId not found in cache */
    fprintf(stderr, "[adu-direct] fileId '%s' not found in cached fileUrls\n", fileId);
    return result;
}

ADUC_Result2 AduDirect_HealthCheck(ADUC_CommHealthStatus* outStatus)
{
    if (outStatus == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_IO, 1);
    }

    if (s_state.connState != ADUC_COMM_STATE_CONNECTED || s_state.curl == NULL)
    {
        *outStatus = ADUC_COMM_HEALTH_DISCONNECTED;
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_IO, 1);
    }

    /* Use a minimal syncConfiguration call as health probe */
    ADUC_Result2 result = SyncConfiguration(false);

    *outStatus = ADUC_RESULT2_IS_SUCCESS(result)
        ? ADUC_COMM_HEALTH_CONNECTED
        : ADUC_COMM_HEALTH_DEGRADED;

    return result;
}

ADUC_Result2 AduDirect_RegisterCallback(ADUC_CommEventType event, ADUC_CommCallback cb, void* ctx)
{
    (void)event;
    (void)cb;
    (void)ctx;

    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 AduDirect_SendDiagnostics(const void* payload, size_t payloadLen)
{
    (void)payload;
    (void)payloadLen;

    return ADUC_RESULT2_SUCCESS;
}

/* -------------------------------------------------------------------------- */
/*                          Extension descriptor export                        */
/* -------------------------------------------------------------------------- */

static ADUC_CommunicationVtable s_vtable = {
    .structVersion = 1,
    .Connect = AduDirect_Connect,
    .Disconnect = AduDirect_Disconnect,
    .GetConnectionState = AduDirect_GetConnectionState,
    .Poll = AduDirect_Poll,
    .RegisterCallback = AduDirect_RegisterCallback,
    .ReportState = AduDirect_ReportState,
    .ReportResult = AduDirect_ReportResult,
    .GetDownloadUrl = AduDirect_GetDownloadUrl,
    .SendDiagnostics = AduDirect_SendDiagnostics,
    .HealthCheck = AduDirect_HealthCheck,
};

static ADUC_ExtensionDescriptor s_descriptor = {
    .structVersion = 1,
    .id = "adu-direct-comm",
    .name = "ADU Direct Communication Provider",
    .version = "2.0.0",
    .type = ADUC_EXT_TYPE_COMMUNICATION,
    .vtable = &s_vtable,
    .Initialize = AduDirect_Initialize,
    .Uninitialize = AduDirect_Uninitialize,
};

ADUC_DECLARE_EXTENSION(s_descriptor)
