/**
 * @file download_service.c
 * @brief Download Service implementation.
 */
#include "aduc/download_service.h"
#include "aduc/content_downloader_vtable.h"
#include "aduc/content_processor_vtable.h"
#include "aduc/extension_descriptor.h"
#include "aduc/extension_loader.h"
#include "aduc/file_info.h"
#include "aduc/log_writer.h"

#include <errno.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct ADUC_DownloadService
{
    ADUC_ExtensionRegistryHandle registry;
    ADUC_DownloadServiceConfig config;
    bool cancelRequested;
};

// ─── Helpers ─────────────────────────────────────────────────────────────────

static ADUC_Result2 compute_sha256(const char* filePath, char* hashBuf, size_t hashBufLen)
{
    if (!filePath || !hashBuf || hashBufLen < 65)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 1);
    }

    FILE* f = fopen(filePath, "rb");
    if (!f)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 2);
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);

    unsigned char buf[65536];
    size_t bytesRead;
    while ((bytesRead = fread(buf, 1, sizeof(buf), f)) > 0)
    {
        EVP_DigestUpdate(ctx, buf, bytesRead);
    }
    fclose(f);

    unsigned char hash[32];
    unsigned int hashLen = 0;
    EVP_DigestFinal_ex(ctx, hash, &hashLen);
    EVP_MD_CTX_free(ctx);

    for (unsigned int i = 0; i < hashLen; i++)
    {
        snprintf(hashBuf + (i * 2), 3, "%02x", hash[i]);
    }
    hashBuf[64] = '\0';

    return ADUC_RESULT2_SUCCESS;
}

static const ADUC_DownloaderVtable* find_downloader(
    ADUC_DownloadService* service,
    const char* uri)
{
    // If preferred downloader is set, try it first
    if (service->config.preferredDownloader && service->config.preferredDownloader[0])
    {
        const ADUC_ExtensionDescriptor* ext =
            ADUC_ExtensionRegistry_FindByCapability(service->registry, service->config.preferredDownloader);
        if (ext && ext->type == ADUC_EXT_TYPE_DOWNLOADER)
        {
            const ADUC_DownloaderVtable* vt = (const ADUC_DownloaderVtable*)ext->vtable;
            if (vt->CanHandle(uri))
            {
                return vt;
            }
        }
    }

    // Auto-select: iterate all downloaders, pick first that can handle the URI
    size_t count = 0;
    const ADUC_ExtensionDescriptor** all =
        ADUC_ExtensionRegistry_FindAllByType(service->registry, ADUC_EXT_TYPE_DOWNLOADER, &count);

    if (all)
    {
        for (size_t i = 0; i < count; i++)
        {
            const ADUC_DownloaderVtable* vt = (const ADUC_DownloaderVtable*)all[i]->vtable;
            if (vt->CanHandle(uri))
            {
                free(all);
                return vt;
            }
        }
        free(all);
    }

    return NULL;
}

/**
 * @brief Find a content processor extension matching the given capability string.
 */
static const ADUC_ContentProcessorVtable* find_content_processor(
    ADUC_DownloadService* service,
    const char* capability)
{
    const ADUC_ExtensionDescriptor* ext =
        ADUC_ExtensionRegistry_FindByCapability(service->registry, capability);
    if (ext && ext->type == ADUC_EXT_TYPE_CONTENT_PROCESSOR)
    {
        return (const ADUC_ContentProcessorVtable*)ext->vtable;
    }
    return NULL;
}

/**
 * @brief Run a content processor on a downloaded file, then re-validate the hash.
 *
 * The processor transforms downloadedPath into processedPath. After processing,
 * the hash of the processed output is validated against fileInfo->hashValue to
 * prevent tampered content injection through a malicious processor.
 */
static ADUC_Result2 run_content_processor(
    ADUC_DownloadService* service,
    const ADUC_ContentProcessorVtable* processor,
    const char* downloadedPath,
    const char* processedPath,
    ADUC_FileInfo* fileInfo)
{
    ADUC_Log_WriteText(ADUC_LOG_INFO, "DownloadService",
        "Running content processor for handler: %s", fileInfo->downloadHandlerId);

    // Use FILE mode processing
    ADUC_Result2 result = processor->ProcessFile(downloadedPath, processedPath, NULL, NULL);
    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        ADUC_Log_WriteText(ADUC_LOG_ERROR, "DownloadService",
            "Content processor failed for %s", fileInfo->fileName);
        unlink(processedPath);
        return result;
    }

    // Re-validate hash on processed output (security: processors can't inject tampered content)
    if (fileInfo->hashValue && fileInfo->hashValue[0])
    {
        char computedHash[65];
        result = compute_sha256(processedPath, computedHash, sizeof(computedHash));
        if (ADUC_RESULT2_IS_FAILURE(result))
        {
            unlink(processedPath);
            return result;
        }

        if (strcasecmp(computedHash, fileInfo->hashValue) != 0)
        {
            ADUC_Log_WriteText(ADUC_LOG_ERROR, "DownloadService",
                "Post-process hash mismatch for %s: expected=%s got=%s",
                fileInfo->fileName, fileInfo->hashValue, computedHash);
            unlink(processedPath);
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_AUTH, 2);
        }

        ADUC_Log_WriteText(ADUC_LOG_INFO, "DownloadService",
            "Post-process hash verified for %s", fileInfo->fileName);
    }

    // Remove the original downloaded file (the delta/compressed input)
    unlink(downloadedPath);

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 apply_file_properties(const char* filePath, const ADUC_FileProperties* props)
{
    if (!props) return ADUC_RESULT2_SUCCESS;

    if (props->permissions && props->permissions[0])
    {
        mode_t mode = (mode_t)strtol(props->permissions, NULL, 8);
        if (chmod(filePath, mode) != 0)
        {
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 10);
        }
    }
    else if (props->executable)
    {
        struct stat st;
        if (stat(filePath, &st) == 0)
        {
            chmod(filePath, st.st_mode | 0111);
        }
    }

    // Owner/group setting requires root or CAP_CHOWN — best effort
    (void)props->owner;
    (void)props->group;

    return ADUC_RESULT2_SUCCESS;
}

// ─── Public API ─────────────────────────────────────────────────────────────

ADUC_Result2 ADUC_DownloadService_Create(
    ADUC_DownloadService** outService,
    ADUC_ExtensionRegistry* registry,
    const ADUC_DownloadServiceConfig* config)
{
    if (!outService || !registry)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 1);
    }

    ADUC_DownloadService* svc = calloc(1, sizeof(ADUC_DownloadService));
    if (!svc)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_RESOURCE, 1);
    }

    svc->registry = (ADUC_ExtensionRegistryHandle)registry;
    if (config)
    {
        svc->config = *config;
    }
    else
    {
        // Defaults
        svc->config.maxRetries = 3;
        svc->config.retryBackoffSec = 5;
        svc->config.connectTimeoutSec = 30;
        svc->config.transferTimeoutSec = 600;
        svc->config.maxConcurrent = 2;
        svc->config.tempDir = "/var/lib/adu/downloads/tmp";
        svc->config.stateDir = "/var/lib/adu/downloads/.state";
    }

    // Ensure temp dir exists
    mkdir(svc->config.tempDir, 0755);

    *outService = svc;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_DownloadService_DownloadFile(
    ADUC_DownloadService* service,
    ADUC_FileInfo* fileInfo,
    const char* destDir,
    const ADUC_DownloadCallbacks* callbacks)
{
    if (!service || !fileInfo || !destDir)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 2);
    }

    const char* uri = fileInfo->downloadUrl;
    if (!uri || !uri[0])
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 3);
    }

    // Find appropriate downloader
    const ADUC_DownloaderVtable* downloader = find_downloader(service, uri);
    if (!downloader)
    {
        ADUC_Log_WriteText(ADUC_LOG_ERROR, "DownloadService", "No downloader can handle URI: %s", uri);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_CONFIG, 1);
    }

    // Build destination path
    char destPath[1024];
    if (fileInfo->targetProperties.path && fileInfo->targetProperties.path[0])
    {
        snprintf(destPath, sizeof(destPath), "%s", fileInfo->targetProperties.path);
    }
    else
    {
        snprintf(destPath, sizeof(destPath), "%s/%s", destDir, fileInfo->fileName);
    }

    // Check for existing partial download (resume)
    uint64_t offset = 0;
    struct stat st;
    char tempPath[1024];
    snprintf(tempPath, sizeof(tempPath), "%s/.%s.partial", destDir, fileInfo->fileName);

    if (stat(tempPath, &st) == 0 && st.st_size > 0)
    {
        offset = (uint64_t)st.st_size;
        ADUC_Log_WriteText(ADUC_LOG_INFO, "DownloadService", "Resuming download at offset %lu", (unsigned long)offset);
    }

    // Download with retry
    ADUC_Result2 result = ADUC_RESULT2_SUCCESS;
    uint32_t attempt = 0;
    uint32_t maxAttempts = service->config.maxRetries + 1;

    while (attempt < maxAttempts)
    {
        if (service->cancelRequested)
        {
            downloader->Suspend();
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 60);
        }

        result = downloader->Download(uri, tempPath, offset, callbacks);
        if (ADUC_RESULT2_IS_SUCCESS(result))
        {
            break;
        }

        attempt++;
        if (attempt < maxAttempts)
        {
            uint32_t delaySec = service->config.retryBackoffSec * (1u << (attempt - 1));
            ADUC_Log_WriteText(ADUC_LOG_WARN, "DownloadService",
                     "Download failed (attempt %u/%u), retrying in %us",
                     attempt, maxAttempts, delaySec);
            sleep(delaySec);

            // Update offset for resume
            if (stat(tempPath, &st) == 0)
            {
                offset = (uint64_t)st.st_size;
            }
        }
    }

    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        ADUC_Log_WriteText(ADUC_LOG_ERROR, "DownloadService", "Download failed after %u attempts", maxAttempts);
        return result;
    }

    // Validate hash
    if (fileInfo->hashValue && fileInfo->hashValue[0])
    {
        char computedHash[65];
        result = compute_sha256(tempPath, computedHash, sizeof(computedHash));
        if (ADUC_RESULT2_IS_FAILURE(result))
        {
            unlink(tempPath);
            return result;
        }

        if (strcasecmp(computedHash, fileInfo->hashValue) != 0)
        {
            ADUC_Log_WriteText(ADUC_LOG_ERROR, "DownloadService",
                     "Hash mismatch for %s: expected=%s got=%s",
                     fileInfo->fileName, fileInfo->hashValue, computedHash);
            unlink(tempPath);
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_AUTH, 1);
        }

        ADUC_Log_WriteText(ADUC_LOG_INFO, "DownloadService", "Hash verified for %s", fileInfo->fileName);
    }

    // Move temp to final destination
    mkdir(destDir, 0755);
    if (rename(tempPath, destPath) != 0)
    {
        ADUC_Log_WriteText(ADUC_LOG_ERROR, "DownloadService", "Failed to move %s to %s: %s",
                 tempPath, destPath, strerror(errno));
        unlink(tempPath);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 5);
    }

    // Run content processor if downloadHandlerId specifies one
    if (fileInfo->downloadHandlerId && fileInfo->downloadHandlerId[0])
    {
        const ADUC_ContentProcessorVtable* processor =
            find_content_processor(service, fileInfo->downloadHandlerId);
        if (processor)
        {
            char processedPath[1034];
            snprintf(processedPath, sizeof(processedPath), "%s.processed", destPath);

            result = run_content_processor(service, processor, destPath, processedPath, fileInfo);
            if (ADUC_RESULT2_IS_FAILURE(result))
            {
                return result;
            }

            // Replace original with processed output
            if (rename(processedPath, destPath) != 0)
            {
                ADUC_Log_WriteText(ADUC_LOG_ERROR, "DownloadService",
                    "Failed to move processed file %s to %s: %s",
                    processedPath, destPath, strerror(errno));
                unlink(processedPath);
                return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 6);
            }
        }
        else
        {
            ADUC_Log_WriteText(ADUC_LOG_WARN, "DownloadService",
                "No content processor found for handler: %s", fileInfo->downloadHandlerId);
        }
    }

    // Apply file properties
    result = apply_file_properties(destPath, &fileInfo->targetProperties);

    // Set localPath on fileInfo for downstream use
    fileInfo->localPath = strdup(destPath);

    ADUC_Log_WriteText(ADUC_LOG_INFO, "DownloadService", "Downloaded: %s -> %s", uri, destPath);
    return result;
}

ADUC_Result2 ADUC_DownloadService_DownloadFiles(
    ADUC_DownloadService* service,
    ADUC_FileInfo* fileInfos,
    size_t count,
    const char* destDir,
    const ADUC_DownloadCallbacks* callbacks)
{
    if (!service || !fileInfos || count == 0)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 2);
    }

    ADUC_Result2 lastError = ADUC_RESULT2_SUCCESS;
    size_t successCount = 0;

    for (size_t i = 0; i < count; i++)
    {
        if (service->cancelRequested)
        {
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 60);
        }

        ADUC_Result2 r = ADUC_DownloadService_DownloadFile(service, &fileInfos[i], destDir, callbacks);
        if (ADUC_RESULT2_IS_SUCCESS(r))
        {
            successCount++;
        }
        else
        {
            lastError = r;
            ADUC_Log_WriteText(ADUC_LOG_ERROR, "DownloadService",
                     "Failed to download file %zu/%zu: %s", i + 1, count, fileInfos[i].fileName);
        }
    }

    if (successCount < count)
    {
        return lastError;
    }
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_DownloadService_ResumeAll(ADUC_DownloadService* service)
{
    (void)service;
    // TODO: Scan state_dir for incomplete downloads and resume them
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_DownloadService_CancelAll(ADUC_DownloadService* service)
{
    if (!service) return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 1);
    service->cancelRequested = true;
    return ADUC_RESULT2_SUCCESS;
}

void ADUC_DownloadService_Destroy(ADUC_DownloadService* service)
{
    if (service)
    {
        free(service);
    }
}
