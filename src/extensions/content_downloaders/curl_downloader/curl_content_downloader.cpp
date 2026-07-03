/**
 * @file curl_content_downloader.cpp
 * @brief Content Downloader Extension using curl command.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/content_downloader_extension.hpp"
#include "aduc/contract_utils.h"
#include "aduc/hash_utils.h"
#include "aduc/logging.h"
#include "aduc/process_utils.hpp" // for ADUC_LaunchChildProcess

#include <sstream>
#include <sys/stat.h> // for stat
#include <vector>

// keep this last to minimize chance to interfere with system header includes.
#include "aduc/aduc_banned.h"

ADUC_Result Download_curl(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int timeoutInSeconds,
    ADUC_DownloadProgressCallback downloadProgressCallback)
{
    UNREFERENCED_PARAMETER(timeoutInSeconds);
    ADUC_Result result = { ADUC_Result_Failure };
    SHAversion algVersion;
    std::vector<std::string> args;
    std::string output;
    int exitCode = 1;
    std::stringstream fullFilePath;
    bool reportProgress = false;

    if (entity == nullptr)
    {
        result.ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_INVALID_FILE_ENTITY;
        goto done;
    }

    if (entity->DownloadUri == nullptr || *entity->DownloadUri == 0)
    {
        result.ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_INVALID_DOWNLOAD_URI;
        goto done;
    }

    fullFilePath << workFolder << "/" << entity->TargetFilename;

    Log_Info(
        "Downloading File '%s' from '%s' to '%s'",
        entity->TargetFilename,
        entity->DownloadUri,
        fullFilePath.str().c_str());

    // -L (or --location). Handle 3xx redirects. See https://curl.se/docs/manpage.html#-L
    args.emplace_back("-L");

    // -C (or --continue-at). The hyphen after, i.e. '-C -', allows auto-resuming the transfer. See https://curl.se/docs/manpage.html#-C
    args.emplace_back("-C");
    args.emplace_back("-");

    // -o (or --output). Output to file instead of stdout. See https://curl.se/docs/manpage.html#-o
    // NOTE: -O (or --remote-name) is not needed as we already have an /absolute/ file path.
    args.emplace_back("-o");
    args.emplace_back(fullFilePath.str().c_str());

    // Finally, tack the url onto the end
    args.emplace_back(entity->DownloadUri);

    exitCode = ADUC_LaunchChildProcess("/usr/bin/curl", args, output);

    if (exitCode == 0)
    {
        result = { ADUC_Result_Download_Success };
    }
    else
    {
        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = ADUC_ERROR_CURL_DOWNLOADER_EXTERNAL_FAILURE(exitCode);
        reportProgress = true;
        goto done;
    }

    Log_Info("Download output:: \n%s", output.c_str());

done:

    if (reportProgress && (downloadProgressCallback != nullptr))
    {
        if (IsAducResultCodeSuccess(result.ResultCode))
        {
            struct stat st;
            const off_t fileSize{ (stat(fullFilePath.str().c_str(), &st) == 0) ? st.st_size : 0 };
            downloadProgressCallback(
                workflowId, entity->FileId, ADUC_DownloadProgressState_Completed, fileSize, entity->SizeInBytes);
        }
        else
        {
            downloadProgressCallback(
                workflowId,
                entity->FileId,
                (result.ResultCode == ADUC_Result_Failure_Cancelled) ? ADUC_DownloadProgressState_Cancelled
                                                                     : ADUC_DownloadProgressState_Error,
                0,
                entity->SizeInBytes);
        }
    }

    Log_Info(
        "Download task end. resultCode: %d, extendedCode: %d (0x%X)",
        result.ResultCode,
        result.ExtendedResultCode,
        result.ExtendedResultCode);
    return result;
}
