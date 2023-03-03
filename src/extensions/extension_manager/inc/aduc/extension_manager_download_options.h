#ifndef ADUC_EXTENSION_MANAGER_DOWNLOAD_OPTIONS_H
#define ADUC_EXTENSION_MANAGER_DOWNLOAD_OPTIONS_H

typedef struct tagADUC_ExtensionManager_Download_Options
{
    unsigned int
        maxTimeoutInSeconds; /**< The maximum number of seconds the content downloader should try to keep downloading. */
} ExtensionManager_Download_Options;

#endif // #define ADUC_EXTENSION_MANAGER_DOWNLOAD_OPTIONS_H
