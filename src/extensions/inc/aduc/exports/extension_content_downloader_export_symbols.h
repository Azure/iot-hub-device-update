#ifndef EXTENSION_CONTENT_DOWNLOADER_EXPORT_SYMBOLS_H
#define EXTENSION_CONTENT_DOWNLOADER_EXPORT_SYMBOLS_H

#include <aduc/exports/extension_common_export_symbols.h> // for GetContractInfo__EXPORT_SYMBOL

//
// Content Downloader Extension export symbols.
//
// V1 contract (1.0) symbols:
//   - GetContractInfo (optional, defaults to V1 if absent)
//   - Initialize(const char* initializeData)
//   - Download(...)
//
// V2 contract (2.0) adds:
//   - Initialize(const char* initializeData, ADUC_LOG_SEVERITY logLevel)
//   - Cleanup()  (optional, called before library unload if present)
//

/**
 * @brief Gets the extension contract info.
 *
 * @param[out] contractInfo The extension contract info.
 * @return ADUC_Result The result.
 * @details ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo)
 */
#define CONTENT_DOWNLOADER__GetContractInfo__EXPORT_SYMBOL GetContractInfo__EXPORT_SYMBOL

/**
 * @brief Initializes the content downloader.
 *
 * V1 contract: ADUC_Result Initialize(const char* initializeData)
 * V2 contract: ADUC_Result Initialize(const char* initializeData, ADUC_LOG_SEVERITY logLevel)
 */
#define CONTENT_DOWNLOADER__Initialize__EXPORT_SYMBOL "Initialize"

/**
 * @brief Cleanup logic before library is unloaded (V2 contract only).
 * @details void Cleanup()
 */
#define CONTENT_DOWNLOADER__Cleanup__EXPORT_SYMBOL "Cleanup"

/**
 * @brief The download export.
 *
 * @param entity The file entity.
 * @param workflowId The workflow id.
 * @param workFolder The work folder for the update payloads.
 * @param timeoutInSeconds The maximum number of seconds to wait to receive data whilst network stays up before the download will timeout.
 * @param downloadProgressCallback The download progress callback function.
 * @return ADUC_Result The result.
 * @details
ADUC_Result Download(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int timeoutInSeconds,
    ADUC_DownloadProgressCallback downloadProgressCallback)
 */
#define CONTENT_DOWNLOADER__Download__EXPORT_SYMBOL "Download"

#endif // EXTENSION_CONTENT_DOWNLOADER_EXPORT_SYMBOLS_H
