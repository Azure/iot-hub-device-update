
#include <aduc/result.h> // for ADUC_Result
#include <aduc/types/download.h> // for ADUC_DownloadProgressCallback
#include <aduc/types/update_content.h> // for ADUC_FileEntity

ADUC_Result Download_curl(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int timeoutInSeconds,
    ADUC_DownloadProgressCallback downloadProgressCallback);

/**
 * @brief Downloads with curl like Download_curl, with an explicit no-progress timeout instead of the config one.
 * @param noProgressTimeoutInSeconds 0 for a single curl attempt. Otherwise transient curl failures are retried,
 * resuming the partial file, until no bytes arrived for this long or timeoutInSeconds elapsed.
 */
ADUC_Result Download_curl_WithNoProgressTimeout(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int timeoutInSeconds,
    unsigned int noProgressTimeoutInSeconds,
    ADUC_DownloadProgressCallback downloadProgressCallback);

/**
 * @brief Returns whether a curl exit code is a network failure worth retrying, e.g. host unreachable or stalled.
 */
bool IsTransientCurlExitCode(int exitCode);
