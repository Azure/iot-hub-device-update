#ifndef DOWNLOAD_HANDLER_FACTORY_H
#define DOWNLOAD_HANDLER_FACTORY_H

typedef void* DownloadHandlerHandle;

DownloadHandlerHandle ADUC_DownloadHandlerFactory_LoadDownloadHandler(const char* downloadHandlerId);
void ADUC_DownloadHandlerFactory_FreeHandle(DownloadHandlerHandle handle);

#endif // DOWNLOAD_HANDLER_FACTORY_H
