/**
 * @file extension_manager.hpp
 * @brief Definition of the ExtensionManager.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_EXTENSION_MANAGER_HPP
#define ADUC_EXTENSION_MANAGER_HPP

#include <aduc/component_enumerator_extension.hpp>
#include <aduc/contract_utils.h>
#include <aduc/extension_manager_download_options.h>
#include <aduc/logging.h> // ADUC_LOG_SEVERITY
#include <aduc/result.h> // ADUC_Result
#include <aduc/types/download.h> // ADUC_DownloadProgressCallback
#include <aduc/types/update_content.h> // ADUC_FileEntity

#include <memory>
#include <string>
#include <unordered_map>

// Default DO retry timeout is 24 hours.
#define DO_RETRY_TIMEOUT_DEFAULT (60 * 60 * 24)

// Forward declaration.
class ContentHandler;

using ADUC_WorkflowHandle = void*;

class ExtensionManager
{
public:
    static ADUC_Result LoadContentDownloaderLibrary(void** contentDownloaderLibrary);
    static ADUC_Result SetContentDownloaderLibrary(void* contentDownloaderLibrary);
    static ADUC_Result GetContentDownloaderContractVersion(ADUC_ExtensionContractInfo* contractInfo);

    static bool IsComponentsEnumeratorRegistered();
    static ADUC_Result LoadComponentEnumeratorLibrary(void** componentEnumerator);
    static ADUC_Result GetComponentEnumeratorContractVersion(ADUC_ExtensionContractInfo* contractInfo);

    static ADUC_Result LoadUpdateContentHandlerExtension(const std::string& updateType, ContentHandler** handler);
    static ADUC_Result SetUpdateContentHandlerExtension(const std::string& updateType, ContentHandler* handler);

    static void Uninit();

    /**
     * @brief Returns all components information in JSON format.
     * @param[out] outputComponentsData An output string containing components data.
     */
    static ADUC_Result GetAllComponents(std::string& outputComponentsData);

    /**
     * @brief Selects component(s) matching specified @p selector.
     * @param selector A JSON string contains name-value pairs used for selecting components.
     * @param[out] outputComponentsData An output string containing components data.
     */
    static ADUC_Result SelectComponents(const std::string& selector, std::string& outputComponentsData);

    /**
     * @brief Initialize Content Downloader extension.
     * @param[in] initializeData A string contains downloader initialization data.
     * @return ADUC_Result
     */
    static ADUC_Result InitializeContentDownloader(const char* initializeData);

    /**
     * @brief
     *
     * @param entity An #ADUC_FileEntity object with information of the file to be downloaded.
     * @param workflowHandle The workflow handle opaque object for per-workflow workflow data.
     * @param downloadOptions The download options.
     * @param downloadProgressCallback A download progress reporting callback.
     * @return ADUC_Result
     */
    static ADUC_Result Download(
        const ADUC_FileEntity* entity,
        ADUC_WorkflowHandle workflowHandle,
        ExtensionManager_Download_Options* downloadOptions,
        ADUC_DownloadProgressCallback downloadProgressCallback);

private:
    static void UnloadAllUpdateContentHandlers();
    static void UnloadAllExtensions();

    static void _FreeComponentsDataString(char* componentsJson);

    static ADUC_Result LoadExtensionLibrary(
        const char* extensionName,
        const char* extensionPath,
        const char* extensionSubfolder,
        const char* extensionRegFileName,
        const char* requiredFunction,
        int facilityCode,
        int componentCode,
        void** libHandle);

    static std::unordered_map<std::string, void*> _libs;
    static std::unordered_map<std::string, ContentHandler*> _contentHandlers;
    static void* _contentDownloader;
    static ADUC_ExtensionContractInfo _contentDownloaderContractVersion;
    static void* _componentEnumerator;
    static ADUC_ExtensionContractInfo _componentEnumeratorContractVersion;
};

#endif // ADUC_EXTENSION_MANAGER_HPP
