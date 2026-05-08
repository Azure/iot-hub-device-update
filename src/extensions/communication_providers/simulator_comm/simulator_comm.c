/**
 * @file simulator_comm.c
 * @brief Simulator communication provider for offline/functional testing.
 *
 * Reads deployment manifests from a local directory, returns file:// URIs
 * for the file side-loader, and writes results to local JSON files.
 * Enables full E2E testing without any cloud dependency.
 *
 * Capability: "adu/communication:simulator"
 */
#include "aduc/communication_vtable.h"
#include "aduc/extension_descriptor.h"
#include "aduc/extension_types.h"

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define SIM_MAX_PATH 2048
#define SIM_MAX_PAYLOAD (512 * 1024) // 512KB max manifest size

// ─── Configuration ──────────────────────────────────────────────────────────

static struct
{
    char manifestDir[SIM_MAX_PATH];
    char contentDir[SIM_MAX_PATH];
    char resultDir[SIM_MAX_PATH];
    bool autoDeploy;
    uint32_t pollDelaySec;

    // State
    ADUC_CommConnectionState connState;
    int manifestIndex;  // Which manifest to serve next
    char** manifestFiles;
    int manifestFileCount;
} s_sim = {
    .manifestDir = "/opt/adu/test/manifests",
    .contentDir = "/opt/adu/test/payloads",
    .resultDir = "/opt/adu/test/results",
    .autoDeploy = true,
    .pollDelaySec = 1,
    .connState = ADUC_COMM_STATE_DISCONNECTED,
    .manifestIndex = 0,
    .manifestFiles = NULL,
    .manifestFileCount = 0,
};

// ─── Helpers ────────────────────────────────────────────────────────────────

static void scan_manifest_dir(void)
{
    // Free previous
    if (s_sim.manifestFiles)
    {
        for (int i = 0; i < s_sim.manifestFileCount; i++)
        {
            free(s_sim.manifestFiles[i]);
        }
        free(s_sim.manifestFiles);
        s_sim.manifestFiles = NULL;
        s_sim.manifestFileCount = 0;
    }

    DIR* dir = opendir(s_sim.manifestDir);
    if (!dir) return;

    // Count .json files
    struct dirent* entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL)
    {
        size_t len = strlen(entry->d_name);
        if (len > 5 && strcmp(entry->d_name + len - 5, ".json") == 0)
        {
            count++;
        }
    }

    if (count == 0)
    {
        closedir(dir);
        return;
    }

    s_sim.manifestFiles = calloc((size_t)count, sizeof(char*));
    rewinddir(dir);

    int idx = 0;
    while ((entry = readdir(dir)) != NULL && idx < count)
    {
        size_t len = strlen(entry->d_name);
        if (len > 5 && strcmp(entry->d_name + len - 5, ".json") == 0)
        {
            char path[SIM_MAX_PATH];
            snprintf(path, sizeof(path), "%s/%s", s_sim.manifestDir, entry->d_name);
            s_sim.manifestFiles[idx] = strdup(path);
            idx++;
        }
    }
    s_sim.manifestFileCount = idx;
    closedir(dir);
}

static char* read_file_contents(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0 || size > SIM_MAX_PAYLOAD)
    {
        fclose(f);
        return NULL;
    }

    char* buf = malloc((size_t)size + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }

    size_t bytesRead = fread(buf, 1, (size_t)size, f);
    buf[bytesRead] = '\0';
    fclose(f);
    return buf;
}

static bool write_file_contents(const char* path, const char* content)
{
    // Ensure directory exists
    char dirPath[SIM_MAX_PATH];
    snprintf(dirPath, sizeof(dirPath), "%s", path);
    char* lastSlash = strrchr(dirPath, '/');
    if (lastSlash)
    {
        *lastSlash = '\0';
        mkdir(dirPath, 0755);
    }

    FILE* f = fopen(path, "w");
    if (!f) return false;
    fputs(content, f);
    fclose(f);
    return true;
}

// ─── Communication Vtable Implementation ────────────────────────────────────

static ADUC_Result2 SimComm_Connect(const ADUC_CommConfig* config)
{
    (void)config;
    s_sim.connState = ADUC_COMM_STATE_CONNECTED;

    // Read config overrides from environment (for testing convenience)
    const char* env;
    if ((env = getenv("ADUC_SIM_MANIFEST_DIR")) != NULL)
    {
        snprintf(s_sim.manifestDir, sizeof(s_sim.manifestDir), "%s", env);
    }
    if ((env = getenv("ADUC_SIM_CONTENT_DIR")) != NULL)
    {
        snprintf(s_sim.contentDir, sizeof(s_sim.contentDir), "%s", env);
    }
    if ((env = getenv("ADUC_SIM_RESULT_DIR")) != NULL)
    {
        snprintf(s_sim.resultDir, sizeof(s_sim.resultDir), "%s", env);
    }

    scan_manifest_dir();
    return ADUC_RESULT2_SUCCESS;
}

static void SimComm_Disconnect(void)
{
    s_sim.connState = ADUC_COMM_STATE_DISCONNECTED;
}

static ADUC_CommConnectionState SimComm_GetConnectionState(void)
{
    return s_sim.connState;
}

static ADUC_Result2 SimComm_Poll(ADUC_CommMessage* outMsg, uint32_t timeoutMs)
{
    (void)timeoutMs;

    if (!outMsg)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_STATE, 1);
    }

    memset(outMsg, 0, sizeof(*outMsg));

    if (s_sim.manifestIndex >= s_sim.manifestFileCount)
    {
        // No more manifests — re-scan directory for new files
        scan_manifest_dir();
        if (s_sim.manifestIndex >= s_sim.manifestFileCount)
        {
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_STATE, 10); // No deployment
        }
    }

    if (s_sim.pollDelaySec > 0 && !s_sim.autoDeploy)
    {
        sleep(s_sim.pollDelaySec);
    }

    // Read next manifest
    char* manifest = read_file_contents(s_sim.manifestFiles[s_sim.manifestIndex]);
    if (!manifest)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_IO, 1);
    }

    outMsg->type = ADUC_COMM_EVENT_DEPLOYMENT_AVAILABLE;
    outMsg->payload = manifest;  // Caller must free
    outMsg->payloadLen = strlen(manifest);
    outMsg->correlationId = "sim-correlation-001";

    s_sim.manifestIndex++;

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SimComm_RegisterCallback(
    ADUC_CommEventType event, ADUC_CommCallback cb, void* ctx)
{
    // Simulator is pull-based only — callbacks not needed
    (void)event; (void)cb; (void)ctx;
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SimComm_ReportState(const ADUC_AgentState* state)
{
    if (!state) return ADUC_RESULT2_SUCCESS;

    char path[SIM_MAX_PATH];
    snprintf(path, sizeof(path), "%s/agent-status.json", s_sim.resultDir);
    write_file_contents(path, state->stateJson ? state->stateJson : "{}");
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SimComm_ReportResult(const ADUC_DeploymentResult2* result)
{
    if (!result || !result->workflowId)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_STATE, 2);
    }

    // Write result as JSON
    char path[SIM_MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s.json", s_sim.resultDir, result->workflowId);

    char json[4096];
    snprintf(json, sizeof(json),
             "{\n"
             "  \"workflowId\": \"%s\",\n"
             "  \"lastInstallResult\": {\n"
             "    \"outcome\": \"%s\",\n"
             "    \"origin\": \"%s\",\n"
             "    \"resultCode\": %lld,\n"
             "    \"extendedResultCodes\": \"%s\",\n"
             "    \"resultDetails\": \"%s\"\n"
             "  }\n"
             "}\n",
             result->workflowId,
             ADUC_Outcome_ToString(result->outcome),
             ADUC_Origin_ToString(result->origin),
             (long long)result->resultCode,
             result->extendedResultCodes ? result->extendedResultCodes : "",
             result->resultDetails ? result->resultDetails : "");

    write_file_contents(path, json);
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SimComm_GetDownloadUrl(
    const char* fileId, char* urlBuf, size_t urlBufLen)
{
    if (!fileId || !urlBuf || urlBufLen == 0)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_STATE, 3);
    }

    // Return file:// URI pointing to content directory
    snprintf(urlBuf, urlBufLen, "file://%s/%s", s_sim.contentDir, fileId);
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SimComm_SendDiagnostics(const void* payload, size_t payloadLen)
{
    if (!payload || payloadLen == 0) return ADUC_RESULT2_SUCCESS;

    char path[SIM_MAX_PATH];
    snprintf(path, sizeof(path), "%s/diagnostics.json", s_sim.resultDir);

    FILE* f = fopen(path, "w");
    if (f)
    {
        fwrite(payload, 1, payloadLen, f);
        fclose(f);
    }
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SimComm_HealthCheck(ADUC_CommHealthStatus* outStatus)
{
    if (outStatus)
    {
        *outStatus = ADUC_COMM_HEALTH_CONNECTED;
    }
    return ADUC_RESULT2_SUCCESS;
}

// ─── Extension Descriptor ───────────────────────────────────────────────────

static ADUC_Result2 SimComm_ExtInit(const ADUC_ExtensionContext* ctx)
{
    (void)ctx;
    return ADUC_RESULT2_SUCCESS;
}

static void SimComm_ExtUninit(void)
{
    if (s_sim.manifestFiles)
    {
        for (int i = 0; i < s_sim.manifestFileCount; i++)
        {
            free(s_sim.manifestFiles[i]);
        }
        free(s_sim.manifestFiles);
        s_sim.manifestFiles = NULL;
    }
}

static const ADUC_CommunicationVtable s_simCommVtable = {
    .structVersion = 1,
    .Connect = SimComm_Connect,
    .Disconnect = SimComm_Disconnect,
    .GetConnectionState = SimComm_GetConnectionState,
    .Poll = SimComm_Poll,
    .RegisterCallback = SimComm_RegisterCallback,
    .ReportState = SimComm_ReportState,
    .ReportResult = SimComm_ReportResult,
    .GetDownloadUrl = SimComm_GetDownloadUrl,
    .SendDiagnostics = SimComm_SendDiagnostics,
    .HealthCheck = SimComm_HealthCheck,
};

static const char* s_capabilities[] = { "adu/communication:simulator", NULL };

static const ADUC_ExtensionDescriptor s_descriptor = {
    .structVersion = ADUC_EXTENSION_DESCRIPTOR_VERSION,
    .id = "microsoft.adu.comm.simulator",
    .name = "ADU Simulator Communication Provider",
    .version = "2.0.0",
    .type = ADUC_EXT_TYPE_COMMUNICATION,
    .minHostApiVersion = ADUC_HOST_API_VERSION,
    .Initialize = SimComm_ExtInit,
    .Uninitialize = SimComm_ExtUninit,
    .vtable = &s_simCommVtable,
    .capabilities = s_capabilities,
};

ADUC_DECLARE_EXTENSION(s_descriptor)
