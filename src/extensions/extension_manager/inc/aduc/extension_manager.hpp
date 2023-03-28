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

#include <dlfcn.h>
#include <string>
#include <unordered_map>

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
     * @brief Handles initialization at begin of download thread execution.
     *
     * @return ADUC_Result The result.
     */
    static ADUC_Result OnDownloadBegin();

    /**
     * @brief Handles uninitialization at end of download thread execution.
     *
     * @return ADUC_Result The result.
     */
    static ADUC_Result OnDownloadEnd();

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

/**
 * @brief Loads the content downloader shared library and returns the exported procedure.
 *
 * @tparam TProc The type of the exported procedure.
 * @param exportName The symbol of the export.
 * @param outProc The output parameter for the exported procedure.
 * @return ADUC_Result The result.
 */
template<typename TProc>
ADUC_Result LoadContentDownloaderLibraryProc(const char* exportName, TProc* outProc)
{
    void* lib = nullptr;
    ADUC_Result result = ExtensionManager::LoadContentDownloaderLibrary(&lib);
    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        *outProc = reinterpret_cast<TProc>(dlsym(lib, exportName));
    }

    return result;
}

#endif // ADUC_EXTENSION_MANAGER_HPP
