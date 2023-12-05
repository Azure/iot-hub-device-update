#ifndef DOWNLOAD_HANDLER_FACTORY_H
#define DOWNLOAD_HANDLER_FACTORY_H

#include <aduc/c_utils.h>

EXTERN_C_BEGIN

typedef void* DownloadHandlerHandle;

/**
 * @brief Gets the plugin handle for a downloadHandlerId, which may involve loading the plugin.
 *
 * @param downloadHandlerId The download handler id from the update metadata of an update payload file.
 * @return DownloadHandlerHandle The opaque handle to the download handler plugin.
 */
DownloadHandlerHandle ADUC_DownloadHandlerFactory_LoadDownloadHandler(const char* downloadHandlerId);

EXTERN_C_END

#endif // DOWNLOAD_HANDLER_FACTORY_H
