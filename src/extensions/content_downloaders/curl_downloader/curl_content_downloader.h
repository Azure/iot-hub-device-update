
#include <aduc/result.h> // for ADUC_Result
#include <aduc/types/download.h> // for ADUC_DownloadProgressCallback
#include <aduc/types/update_content.h> // for ADUC_FileEntity

ADUC_Result Download_curl(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int retryTimeout,
    ADUC_DownloadProgressCallback downloadProgressCallback);
