#ifndef ADUC_EXTENSION_MANAGER_DOWNLOAD_OPTIONS_H
#define ADUC_EXTENSION_MANAGER_DOWNLOAD_OPTIONS_H

#include <aduc/c_utils.h>

// Default Content Downloader Download Timeout of 8 hours.
#define CONTENT_DOWNLOADER_MAX_TIMEOUT_IN_MINUTES_DEFAULT (8 * 60)

typedef struct tagADUC_ExtensionManager_Download_Options
{
    unsigned int
        timeoutInMinutes; /**< The maximum number of minutes the content downloader should wait for the download to complete(whilst the network interface stays up). */
} ExtensionManager_Download_Options;

EXTERN_C_BEGIN
extern ExtensionManager_Download_Options Default_ExtensionManager_Download_Options;
EXTERN_C_END

#endif // #define ADUC_EXTENSION_MANAGER_DOWNLOAD_OPTIONS_H
