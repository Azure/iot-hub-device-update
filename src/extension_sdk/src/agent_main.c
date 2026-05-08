/**
 * @file agent_main.c
 * @brief ADU Gen2 agent daemon - loads config, loads extensions, polls for
 *        deployments, and orchestrates update workflows.
 */

#include "aduc/agent.h"
#include "aduc/communication_vtable.h"
#include "aduc/config_reader.h"
#include "aduc/content_downloader_vtable.h"
#include "aduc/download_service.h"
#include "aduc/extension_descriptor.h"
#include "aduc/extension_loader.h"
#include "aduc/file_info.h"
#include "aduc/log_writer.h"
#include "aduc/manifest_parser.h"
#include "aduc/workflow_engine.h"

#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define ADUC_DEFAULT_CONFIG_PATH "/etc/adu/adu-agent.conf"
#define ADUC_DEFAULT_POLL_INTERVAL_SEC 30
#define ADUC_MS_PER_SEC 1000
#define ADUC_DOWNLOAD_DIR "/tmp/adu-downloads"

static volatile sig_atomic_t g_shutdownRequested = 0;

/* -------------------------------------------------------------------------- */
/*  Download helper — uses Download Service + extension downloaders            */
/* -------------------------------------------------------------------------- */

/**
 * @brief Download all files referenced in the manifest using the Download Service.
 *
 * For each file, resolves the download URL via the comm provider's GetDownloadUrl,
 * then delegates to the Download Service which selects the appropriate downloader
 * extension (curl, file side-loader, etc.), validates hashes, and sets file properties.
 */
static void download_manifest_files(
    ADUC_DownloadService* dlService,
    const ADUC_CommunicationVtable* commVtable,
    ADUC_ParsedManifest* manifest)
{
    if (!manifest || manifest->fileCount == 0) return;

    /* Create download directory */
    char dlDir[256];
    snprintf(dlDir, sizeof(dlDir), "%s/%s", ADUC_DOWNLOAD_DIR, manifest->workflowId);
    mkdir(ADUC_DOWNLOAD_DIR, 0755);
    mkdir(dlDir, 0755);

    for (size_t i = 0; i < manifest->fileCount; i++)
    {
        ADUC_StepFile* f = &manifest->files[i];
        if (f->path != NULL) continue; /* Already downloaded */
        if (f->id == NULL || f->name == NULL) continue;

        /* Resolve download URL via communication provider */
        char urlBuf[2048];
        urlBuf[0] = '\0';
        ADUC_Result2 urlResult = commVtable->GetDownloadUrl(f->id, urlBuf, sizeof(urlBuf));
        if (ADUC_RESULT2_IS_FAILURE(urlResult) || urlBuf[0] == '\0')
        {
            ADUC_Log_WriteText(ADUC_LOG_WARN, "agent",
                "Failed to resolve URL for file: %s (%s)", f->id, f->name);
            continue;
        }

        /* Build ADUC_FileInfo for the Download Service */
        ADUC_FileInfo fileInfo;
        memset(&fileInfo, 0, sizeof(fileInfo));
        fileInfo.fileId = f->id;
        fileInfo.fileName = f->name;
        fileInfo.sizeInBytes = f->size;
        fileInfo.downloadUrl = urlBuf;
        fileInfo.hashAlgorithm = "sha256";
        fileInfo.hashValue = f->sha256;
        fileInfo.targetProperties.executable = true; /* scripts need exec bit */

        /* Download via Download Service (handles retry, hash, resume, file props) */
        ADUC_Result2 dlResult = ADUC_DownloadService_DownloadFile(
            dlService, &fileInfo, dlDir, NULL);

        if (ADUC_RESULT2_IS_SUCCESS(dlResult) && fileInfo.localPath != NULL)
        {
            f->path = fileInfo.localPath; /* Transfer ownership */
            ADUC_Log_WriteText(ADUC_LOG_INFO, "agent",
                "Downloaded: %s → %s", f->id, f->path);
        }
        else
        {
            ADUC_Log_WriteText(ADUC_LOG_WARN, "agent",
                "Failed to download file: %s (%s) code=0x%08x",
                f->id, f->name, dlResult.code);
        }
    }
}

static void signal_handler(int signum)
{
    (void)signum;
    g_shutdownRequested = 1;
}

static void install_signal_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
}

static void print_usage(const char* progname)
{
    fprintf(stderr,
        "Usage: %s [OPTIONS]\n\n"
        "  -c, --config <path>     Config file (default: %s)\n"
        "  -l, --log-level <1-6>   Log level (1=fatal..6=trace, default: 4=info)\n"
        "  -f, --log-file <path>   Binary log file path\n"
        "  -1, --once              Process one deployment then exit\n"
        "  -h, --help              Show this help\n",
        progname, ADUC_DEFAULT_CONFIG_PATH);
}

ADUC_Result2 ADUC_Agent_ParseArgs(int argc, char** argv, ADUC_AgentConfig* config)
{
    if (config == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 1);
    }

    config->configPath = ADUC_DEFAULT_CONFIG_PATH;
    config->logFilePath = NULL;
    config->logLevel = ADUC_LOG_INFO;
    config->runOnce = false;
    config->pollIntervalSec = ADUC_DEFAULT_POLL_INTERVAL_SEC;

    static struct option long_options[] = {
        { "config",    required_argument, NULL, 'c' },
        { "log-level", required_argument, NULL, 'l' },
        { "log-file",  required_argument, NULL, 'f' },
        { "once",      no_argument,       NULL, '1' },
        { "help",      no_argument,       NULL, 'h' },
        { NULL,        0,                 NULL,  0  }
    };

    int opt;
    optind = 1;

    while ((opt = getopt_long(argc, argv, "c:l:f:1h", long_options, NULL)) != -1)
    {
        switch (opt)
        {
            case 'c':
                config->configPath = optarg;
                break;
            case 'l':
            {
                int level = atoi(optarg);
                if (level < (int)ADUC_LOG_FATAL || level > (int)ADUC_LOG_TRACE)
                {
                    fprintf(stderr, "Error: invalid log level '%s'\n", optarg);
                    return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 2);
                }
                config->logLevel = (ADUC_LogLevel)level;
                break;
            }
            case 'f':
                config->logFilePath = optarg;
                break;
            case '1':
                config->runOnce = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 0);
            default:
                print_usage(argv[0]);
                return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 3);
        }
    }

    return ADUC_RESULT2_SUCCESS;
}

/* Workflow callbacks */
static void workflow_progress_cb(
    const char* workflowId,
    const char* stepId,
    const char* phase,
    uint32_t percentComplete,
    void* ctx)
{
    (void)ctx;
    ADUC_Log_WriteText(ADUC_LOG_INFO, "workflow",
        "[%s] step=%s phase=%s progress=%u%%", workflowId, stepId, phase, percentComplete);
}

static void workflow_complete_cb(
    const char* workflowId,
    ADUC_Result2 overallResult,
    const ADUC_StepResult* stepResults,
    size_t stepCount,
    void* ctx)
{
    (void)stepResults;
    (void)stepCount;
    (void)ctx;
    ADUC_Log_WriteText(ADUC_LOG_INFO, "workflow",
        "[%s] complete result=0x%08x", workflowId, overallResult.code);
}

ADUC_Result2 ADUC_Agent_Run(const ADUC_AgentConfig* config)
{
    ADUC_Result2 result = ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_STATE, 1);
    ADUC_ConfigHandle cfgHandle = NULL;
    ADUC_LogWriter* logger = NULL;
    ADUC_ExtensionRegistryHandle registry = NULL;
    const ADUC_CommunicationVtable* commVtable = NULL;
    const ADUC_ExtensionDescriptor* commDesc = NULL;
    ADUC_DownloadService* dlService = NULL;

    if (config == NULL)
    {
        return result;
    }

    install_signal_handlers();

    /* 1. Load configuration */
    result = ADUC_Config_LoadFile(config->configPath, &cfgHandle);
    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        fprintf(stderr, "Error: failed to load config from '%s' (code=0x%08x)\n",
            config->configPath, result.code);
        goto done;
    }

    /* 2. Initialize logger */
    const char* logPath = config->logFilePath;
    if (logPath == NULL)
    {
        logPath = ADUC_Config_GetString(cfgHandle, "agent.log_file");
        if (logPath == NULL)
        {
            logPath = "/var/log/adu/adu-agent.bin";
        }
    }

    ADUC_Log_SetLevel(config->logLevel);
    result = ADUC_Log_Create(logPath, &logger);
    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        fprintf(stderr, "Error: failed to create logger at '%s'\n", logPath);
        goto done;
    }

    ADUC_Log_WriteText(ADUC_LOG_INFO, "agent", "ADU Agent starting (config=%s)", config->configPath);

    /* 2b. Override poll interval from config if present */
    uint32_t effectivePollSec = config->pollIntervalSec;
    {
        int64_t pollVal = 0;
        if (ADUC_RESULT2_IS_SUCCESS(ADUC_Config_GetInt(cfgHandle, "communication.poll_interval_sec", &pollVal))
            && pollVal > 0)
        {
            effectivePollSec = (uint32_t)pollVal;
        }
    }

    /* 3. Create extension registry */
    result = ADUC_ExtensionRegistry_Create(&registry);
    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        ADUC_Log_WriteText(ADUC_LOG_ERROR, "agent", "Failed to create extension registry");
        goto done;
    }

    /* 4. Load extensions from search_path */
    {
        const char* searchPath = ADUC_Config_GetString(cfgHandle, "extensions.search_path");
        if (searchPath != NULL)
        {
            ADUC_Log_WriteText(ADUC_LOG_INFO, "agent", "Scanning extensions from: %s", searchPath);
            result = ADUC_ExtensionRegistry_ScanDirectory(registry, searchPath, NULL);
            if (ADUC_RESULT2_IS_FAILURE(result))
            {
                ADUC_Log_WriteText(ADUC_LOG_WARN, "agent", "No extensions loaded from %s", searchPath);
            }
            else
            {
                ADUC_Log_WriteText(ADUC_LOG_INFO, "agent", "Extensions loaded: %zu",
                    ADUC_ExtensionRegistry_GetCount(registry));
            }
        }
        else
        {
            ADUC_Log_WriteText(ADUC_LOG_WARN, "agent", "No extensions.search_path configured");
        }
    }

    /* 5. Create Download Service */
    {
        ADUC_DownloadServiceConfig dlConfig;
        memset(&dlConfig, 0, sizeof(dlConfig));
        dlConfig.maxRetries = 3;
        dlConfig.retryBackoffSec = 5;
        dlConfig.connectTimeoutSec = 30;
        dlConfig.transferTimeoutSec = 600;
        dlConfig.maxConcurrent = 2;
        dlConfig.tempDir = ADUC_DOWNLOAD_DIR;
        dlConfig.stateDir = ADUC_DOWNLOAD_DIR "/.state";

        result = ADUC_DownloadService_Create(&dlService, registry, &dlConfig);
        if (ADUC_RESULT2_IS_FAILURE(result))
        {
            ADUC_Log_WriteText(ADUC_LOG_ERROR, "agent", "Failed to create download service");
            goto done;
        }
        ADUC_Log_WriteText(ADUC_LOG_INFO, "agent", "Download service initialized");
    }

    /* 6. Find communication provider */
    commDesc = ADUC_ExtensionRegistry_FindByType(registry, ADUC_EXT_TYPE_COMMUNICATION, 0);
    if (commDesc == NULL)
    {
        ADUC_Log_WriteText(ADUC_LOG_ERROR, "agent", "No communication provider extension found");
        result = ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_CONFIG, 1);
        goto done;
    }

    commVtable = (const ADUC_CommunicationVtable*)commDesc->vtable;
    if (commVtable == NULL)
    {
        ADUC_Log_WriteText(ADUC_LOG_ERROR, "agent", "Communication extension has no vtable");
        result = ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_CONFIG, 2);
        goto done;
    }

    /* 7. Connect to service */
    const char* commEndpoint = ADUC_Config_GetString(cfgHandle, "communication.endpoint");
    const char* commDeviceId = ADUC_Config_GetString(cfgHandle, "agent.device_id");
    {
        ADUC_CommConfig commConfig;
        memset(&commConfig, 0, sizeof(commConfig));
        commConfig.endpoint = commEndpoint;
        commConfig.deviceId = commDeviceId;
        commConfig.pollIntervalSec = effectivePollSec;

        result = commVtable->Connect(&commConfig);
        if (ADUC_RESULT2_IS_FAILURE(result))
        {
            ADUC_Log_WriteText(ADUC_LOG_ERROR, "agent", "Failed to connect (code=0x%08x)", result.code);
            goto done;
        }
    }

    ADUC_Log_WriteText(ADUC_LOG_INFO, "agent", "Connected, entering poll loop (interval=%us)",
        effectivePollSec);

    /* 8. Main loop */
    uint32_t pollIntervalMs = effectivePollSec * ADUC_MS_PER_SEC;

    while (!g_shutdownRequested)
    {
        ADUC_CommMessage msg;
        memset(&msg, 0, sizeof(msg));

        result = commVtable->Poll(&msg, pollIntervalMs);
        if (ADUC_RESULT2_IS_FAILURE(result))
        {
            if (g_shutdownRequested) { break; }
            continue;
        }

        if (msg.type != ADUC_COMM_EVENT_DEPLOYMENT_AVAILABLE || msg.payload == NULL)
        {
            continue;
        }

        ADUC_Log_WriteText(ADUC_LOG_INFO, "agent", "Deployment received (%zu bytes)", msg.payloadLen);

        /* Parse manifest */
        ADUC_ParsedManifest* manifest = NULL;
        result = ADUC_Manifest_Parse(msg.payload, msg.payloadLen, &manifest);
        if (ADUC_RESULT2_IS_FAILURE(result))
        {
            ADUC_Log_WriteText(ADUC_LOG_ERROR, "agent", "Manifest parse failed (code=0x%08x)", result.code);
            continue;
        }

        /* Download files via Download Service + extension downloaders */
        download_manifest_files(dlService, commVtable, manifest);

        /* Build deployment from manifest */
        ADUC_Deployment deployment;
        memset(&deployment, 0, sizeof(deployment));
        deployment.workflowId = manifest->workflowId;
        deployment.steps = manifest->steps;
        deployment.stepCount = manifest->stepCount;
        deployment.retryLimit = 1;

        /* Execute workflow */
        ADUC_WorkflowEngineHandle wfHandle = NULL;
        result = ADUC_Workflow_Execute(
            &deployment, registry,
            workflow_progress_cb, workflow_complete_cb,
            NULL, &wfHandle);

        ADUC_Log_WriteText(ADUC_LOG_INFO, "agent", "Workflow finished (code=0x%08x)", result.code);

        /* Report result (v2 wire format) */
        ADUC_DeploymentResult2 depResult;
        memset(&depResult, 0, sizeof(depResult));
        depResult.workflowId = manifest->workflowId;
        depResult.outcome = ADUC_RESULT2_IS_SUCCESS(result)
            ? ADUC_Outcome_Succeeded : ADUC_Outcome_Failed;
        depResult.origin = ADUC_RESULT2_IS_SUCCESS(result)
            ? ADUC_Origin_AgentCore : ADUC_Origin_AgentCore;
        depResult.resultCode = (int64_t)result.code;

        /* Format extendedResultCode as hex string */
        char ercBuf[32];
        snprintf(ercBuf, sizeof(ercBuf), "%08X", result.code);
        depResult.extendedResultCodes = ercBuf;

        commVtable->ReportResult(&depResult);

        if (wfHandle != NULL)
        {
            ADUC_Workflow_Destroy(wfHandle);
        }
        ADUC_Manifest_Free(manifest);

        if (config->runOnce)
        {
            ADUC_Log_WriteText(ADUC_LOG_INFO, "agent", "Run-once mode: exiting");
            break;
        }
    }

    if (g_shutdownRequested)
    {
        ADUC_Log_WriteText(ADUC_LOG_INFO, "agent", "Shutdown signal received");
    }

    result = ADUC_RESULT2_SUCCESS;

done:
    if (dlService != NULL)
    {
        ADUC_DownloadService_Destroy(dlService);
    }
    if (commVtable != NULL)
    {
        commVtable->Disconnect();
    }
    if (registry != NULL)
    {
        ADUC_ExtensionRegistry_Destroy(registry);
    }
    if (logger != NULL)
    {
        ADUC_Log_WriteText(ADUC_LOG_INFO, "agent", "ADU Agent shutting down");
        ADUC_Log_Flush(logger);
        ADUC_Log_Destroy(logger);
    }
    if (cfgHandle != NULL)
    {
        ADUC_Config_Free(cfgHandle);
    }

    return result;
}

int main(int argc, char** argv)
{
    ADUC_AgentConfig config;
    memset(&config, 0, sizeof(config));

    ADUC_Result2 result = ADUC_Agent_ParseArgs(argc, argv, &config);
    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        return (result.code & 0xFFFF) == 0 ? 0 : 1;
    }

    result = ADUC_Agent_Run(&config);
    return ADUC_RESULT2_IS_SUCCESS(result) ? 0 : 1;
}
