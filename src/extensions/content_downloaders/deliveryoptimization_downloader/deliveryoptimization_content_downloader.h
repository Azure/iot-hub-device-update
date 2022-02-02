#ifndef DELIVERYOPTIMIZATION_CONTENT_DOWNLOADER_HELPERS_H
#define DELIVERYOPTIMIZATION_CONTENT_DOWNLOADER_HELPERS_H

#include <aduc/result.h> // for ADUC_Result
#include <aduc/types/download.h> // for ADUC_DownloadProgressCallback
#include <aduc/types/update_content.h> // for ADUC_FileEntity

ADUC_Result do_download(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int retryTimeout,
    ADUC_DownloadProgressCallback downloadProgressCallback);

#endif // DELIVERYOPTIMIZATION_CONTENT_DOWNLOADER_HELPERS_H
