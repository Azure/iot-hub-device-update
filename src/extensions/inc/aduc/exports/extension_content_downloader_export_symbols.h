#ifndef EXTENSION_CONTENT_DOWNLOADER_EXPORT_SYMBOLS_H
#define EXTENSION_CONTENT_DOWNLOADER_EXPORT_SYMBOLS_H

#include <aduc/exports/extension_common_export_symbols.h> // for GetContractInfo__EXPORT_SYMBOL

//
// Content Downloader Extension export V1 symbols.
// These are the V1 symbols that must be implemented by a content downloader extension.
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
 * @param initializeData The initialization data.
 * @return ADUC_Result The result.
 * @details ADUC_Result Initialize(const char* initializeData)
 */
#define CONTENT_DOWNLOADER__Initialize__EXPORT_SYMBOL "Initialize"

/**
 * @brief The OnDownloadBegin export.
 * @return ADUC_Result The result.
 *
 */
#define CONTENT_DOWNLOADER__OnDownloadBegin__EXPORT_SYMBOL "OnDownloadBegin"

/**
 * @brief The OnDownloadEnd export.
 * @return ADUC_Result The result.
 *
 */
#define CONTENT_DOWNLOADER__OnDownloadEnd__EXPORT_SYMBOL "OnDownloadEnd"

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
