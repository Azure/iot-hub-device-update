/**
 * @file extension_manager.cpp
 * @brief Implementation of ExtensionManager.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/component_enumerator_extension.hpp"
#include "aduc/content_downloader_extension.hpp"
#include "aduc/content_handler.hpp"

#include "aduc/c_utils.h"
#include "aduc/exceptions.hpp"
#include "aduc/extension_manager.hpp"
#include "aduc/extension_utils.h"
#include "aduc/hash_utils.h" // for SHAversion
#include "aduc/logging.h"
#include "aduc/parser_utils.h"
#include "aduc/result.h"
#include "aduc/string_utils.hpp"

#include <cstring>
#include <unordered_map>
#include <vector>

#include <azure_c_shared_utility/azure_base64.h>
#include <azure_c_shared_utility/buffer_.h>
#include <azure_c_shared_utility/crt_abstractions.h> // for mallocAndStrcpy_s

// Note: this requires ${CMAKE_DL_LIBS}
#include <dlfcn.h>
#include <unistd.h>

// Static members.
std::unordered_map<std::string, void*> ExtensionManager::_libs;
std::unordered_map<std::string, ContentHandler*> ExtensionManager::_contentHandlers;
void* ExtensionManager::_contentDownloader;
void* ExtensionManager::_componentEnumerator;

STRING_HANDLE FolderNameFromHandlerId(const char* handlerId)
{
    STRING_HANDLE name = STRING_construct(handlerId);
    STRING_replace(name, '/', '_');
    STRING_replace(name, ':', '_');
    return name;
}

/**
 * @brief Loads extension shared library file.
 * @param extensionName An extension name.
 * @param extensionFolder An extension root folder.
 * @param extensionSubfolder An extension sub-folder.
 * @param extensionRegFileName An extension registration file name.
 * @param requiredFunctionName An required function.
 * @param facilityCode Facility code for extended error report.
 * @param componentCode Component code for extended error report.
 * @param libHandle A buffer for storing output extension library handle.
 * @return ADUC_Result contains result code and extended result code.
 */
ADUC_Result ExtensionManager::LoadExtensionLibrary(
    const char* extensionName,
    const char* extensionPath,
    const char* extensionSubfolder,
    const char* extensionRegFileName,
    const char* requiredFunction,
    int facilityCode,
    int componentCode,
    void** libHandle)
{
    ADUC_Result result{ ADUC_GeneralResult_Failure };
    ADUC_FileEntity entity = {};
    SHAversion algVersion;

    std::stringstream path;
    path << extensionPath << "/" << extensionSubfolder << "/" << extensionRegFileName;

    Log_Info("Loading extension '%s'. Reg file : %s", extensionName, path.str().c_str());

    if (libHandle == nullptr)
    {
        Log_Error("Invalid argument(s).");
        result.ExtendedResultCode = ADUC_ERC_EXTENSION_CREATE_FAILURE_INVALID_ARG(facilityCode, componentCode);
        goto done;
    }

    // Try to find cached handler.
    if (ExtensionManager::_libs.count(extensionName) > 0)
    {
        try
        {
            *libHandle = ExtensionManager::_libs.at(extensionName);
            result.ResultCode = ADUC_Result_Success;
            goto done;
        }
        catch (const std::exception& ex)
        {
            Log_Debug("An exception occurred: %s", ex.what());
        }
        catch (...)
        {
            Log_Debug("Unknown exception occurred while try to reuse '%s'", extensionName);
        }
    }

    memset(&entity, 0, sizeof(entity));

    if (!GetExtensionFileEntity(path.str().c_str(), &entity))
    {
        Log_Info("Failed to load extension from '%s'.", path.str().c_str());
        result.ExtendedResultCode = ADUC_ERC_EXTENSION_CREATE_FAILURE_NOT_FOUND(facilityCode, componentCode);
        goto done;
    }

    // Validate file hash.
    if (!ADUC_HashUtils_GetShaVersionForTypeString(
            ADUC_HashUtils_GetHashType(entity.Hash, entity.HashCount, 0), &algVersion))
    {
        Log_Error(
            "FileEntity for %s has unsupported hash type %s",
            entity.TargetFilename,
            ADUC_HashUtils_GetHashType(entity.Hash, entity.HashCount, 0));
        result.ExtendedResultCode = ADUC_ERC_EXTENSION_CREATE_FAILURE_VALIDATE(facilityCode, componentCode);
        goto done;
    }

    if (!ADUC_HashUtils_IsValidFileHash(
            entity.TargetFilename,
            ADUC_HashUtils_GetHashValue(entity.Hash, entity.HashCount, 0),
            algVersion,
            true /* suppressErrorLog */))
    {
        Log_Error("Hash for %s is not valid", entity.TargetFilename);
        result.ExtendedResultCode = ADUC_ERC_EXTENSION_CREATE_FAILURE_VALIDATE(facilityCode, componentCode);
        goto done;
    }

    *libHandle = dlopen(entity.TargetFilename, RTLD_LAZY);

    if (*libHandle == nullptr)
    {
        Log_Error("Cannot load content handler file %s. %s.", entity.TargetFilename, dlerror());
        result.ExtendedResultCode = ADUC_ERC_EXTENSION_CREATE_FAILURE_LOAD(facilityCode, componentCode);
        goto done;
    }

    dlerror(); // Clear any existing error

    // Only check whether required function exist, if specified.
    if (requiredFunction != nullptr && *requiredFunction != '\0')
    {
        void* reqFunc = dlsym(*libHandle, requiredFunction);

        if (reqFunc == nullptr)
        {
            Log_Error("The specified function ('%s') doesn't exist. %s\n", requiredFunction, dlerror());
            result.ExtendedResultCode =
                ADUC_ERC_EXTENSION_FAILURE_REQUIRED_FUNCTION_NOTIMPL(facilityCode, componentCode);
            goto done;
        }
    }

    // Cache the loaded library.
    ExtensionManager::_libs.emplace(extensionName, *libHandle);

    result = { ADUC_Result_Success };

done:
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        if (*libHandle != nullptr)
        {
            dlclose(*libHandle);
            *libHandle = nullptr;
        }
    }

    // Done with file entity.
    ADUC_FileEntity_Uninit(&entity);

    return result;
}

/**
 * @brief Loads UpdateContentHandler for specified @p updateType
 * @param updateType An update type string.
 * @param handler A buffer for storing an output UpdateContentHandler object.
 * @return ADUCResult contains result code and extended result code.
 * */
ADUC_Result
ExtensionManager::LoadUpdateContentHandlerExtension(const std::string& updateType, ContentHandler** handler)
{
    ADUC_Result result = { ADUC_Result_Failure };

    UPDATE_CONTENT_HANDLER_CREATE_PROC createUpdateContentHandlerExtension = nullptr;
    void* libHandle = nullptr;
    STRING_HANDLE folderName = nullptr;
    STRING_HANDLE path = nullptr;

    Log_Info("Loading Update Content Handler for '%s'.", updateType.c_str());

    if (handler == nullptr)
    {
        Log_Error("Invalid argument(s).");
        result.ExtendedResultCode =
            ADUC_ERC_EXTENSION_CREATE_FAILURE_INVALID_ARG(ADUC_FACILITY_EXTENSION_UPDATE_CONTENT_HANDLER, 0);
        goto done;
    }

    // Try to find cached handler.
    *handler = nullptr;
    if (_contentHandlers.count(updateType) > 0)
    {
        try
        {
            *handler = _contentHandlers.at(updateType);
        }
        catch (const std::exception& ex)
        {
            Log_Debug("An exception occurred: %s", ex.what());
        }
        catch (...)
        {
            Log_Debug("Unknown exception occurred while try to reuse a handler for '%s'", updateType.c_str());
        }
    }

    if (IsAducResultCodeSuccess(result.ResultCode) && *handler != nullptr)
    {
        goto done;
    }

    folderName = FolderNameFromHandlerId(updateType.c_str());
    path = STRING_construct_sprintf("%s/%s", ADUC_UPDATE_CONTENT_HANDLER_EXTENSION_DIR, STRING_c_str(folderName));

    result = LoadExtensionLibrary(
        updateType.c_str(),
        ADUC_UPDATE_CONTENT_HANDLER_EXTENSION_DIR,
        STRING_c_str(folderName),
        ADUC_UPDATE_CONTENT_HANDLER_REG_FILENAME,
        "CreateUpdateContentHandlerExtension",
        ADUC_FACILITY_EXTENSION_UPDATE_CONTENT_HANDLER,
        0,
        &libHandle);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    dlerror(); // Clear any existing error

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    createUpdateContentHandlerExtension =
        reinterpret_cast<UPDATE_CONTENT_HANDLER_CREATE_PROC>(dlsym(libHandle, "CreateUpdateContentHandlerExtension"));

    if (createUpdateContentHandlerExtension == nullptr)
    {
        Log_Error("The specified function doesn't exist. %s\n", dlerror());
        result.ExtendedResultCode =
            ADUC_ERC_EXTENSION_FAILURE_REQUIRED_FUNCTION_NOTIMPL(ADUC_FACILITY_EXTENSION_UPDATE_CONTENT_HANDLER, 0);
        goto done;
    }

    try
    {
        *handler = createUpdateContentHandlerExtension(ADUC_Logging_GetLevel());
    }
    catch (const std::exception& ex)
    {
        Log_Debug("An exception occurred while creating update handler: %s", ex.what());
    }
    catch (...)
    {
        Log_Debug("Unknown exception occurred while creating update handler for '%s'", updateType.c_str());
    }

    if (*handler == nullptr)
    {
        result = { ADUC_GeneralResult_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_CREATE_FAILURE_CREATE };
        goto done;
    }

    Log_Debug("Caching new content handler for '%s'.", updateType.c_str());
    _contentHandlers.emplace(updateType, *handler);

    result = { ADUC_GeneralResult_Success };

done:
    if (result.ResultCode == 0)
    {
        if (libHandle != nullptr)
        {
            dlclose(libHandle);
            libHandle = nullptr;
        }
    }

    STRING_delete(folderName);
    STRING_delete(path);

    return result;
}

/**
 * @brief Sets UpdateContentHandler for specified @p updateType
 * @param updateType An update type string.
 * @param handler A ContentHandler object.
 * @return ADUCResult contains result code and extended result code.
 * */
ADUC_Result ExtensionManager::SetUpdateContentHandlerExtension(const std::string& updateType, ContentHandler* handler)
{
    ADUC_Result result = { ADUC_Result_Failure };

    Log_Info("Setting Content Handler for '%s'.", updateType.c_str());

    if (handler == nullptr)
    {
        Log_Error("Invalid argument(s).");
        result.ExtendedResultCode =
            ADUC_ERC_EXTENSION_CREATE_FAILURE_INVALID_ARG(ADUC_FACILITY_EXTENSION_UPDATE_CONTENT_HANDLER, 0);
        goto done;
    }

    // Remove existing one.
    _contentHandlers.erase(updateType);

    _contentHandlers.emplace(updateType, handler);

    result = { ADUC_GeneralResult_Success };

done:
    return result;
}

void ExtensionManager::UnloadAllUpdateContentHandlers()
{
    for (auto& contentHandler : _contentHandlers)
    {
        delete (contentHandler.second); // NOLINT(cppcoreguidelines-owning-memory)
    }

    _contentHandlers.clear();
}

/**
 * @brief This API unloads all handlers then unload all extension libraries.
 *
 */
void ExtensionManager::UnloadAllExtensions()
{
    // Make sure we unload every handlers first.
    UnloadAllUpdateContentHandlers();

    for (auto& lib : _libs)
    {
        dlclose(lib.second);
    }

    _libs.clear();
}

void ExtensionManager::Uninit()
{
    ExtensionManager::UnloadAllExtensions();
}

ADUC_Result ExtensionManager::LoadContentDownloaderLibrary(void** contentDownloaderLibrary)
{
    ADUC_Result result = { ADUC_Result_Failure };
    static const char* functionNames[] = { "Download", "Initialize" };
    void* extensionLib = nullptr;

    if (_contentDownloader != nullptr)
    {
        *contentDownloaderLibrary = _contentDownloader;
        result = { ADUC_GeneralResult_Success };
        goto done;
    }

    result = LoadExtensionLibrary(
        "Content Downloader",
        ADUC_EXTENSIONS_FOLDER,
        ADUC_EXTENSIONS_SUBDIR_CONTENT_DOWNLOADER,
        ADUC_EXTENSION_REG_FILENAME,
        functionNames[0],
        ADUC_FACILITY_EXTENSION_CONTENT_DOWNLOADER,
        0,
        &extensionLib);

    if (IsAducResultCodeFailure(result.ResultCode) || extensionLib == nullptr)
    {
        goto done;
    }

    for (auto& functionName : functionNames)
    {
        dlerror(); // Clear any existing error
        void* downloadFunc = dlsym(extensionLib, functionName);

        if (downloadFunc == nullptr)
        {
            Log_Error("The specified function ('%s') doesn't exist. %s\n", functionName, dlerror());
            result = { .ResultCode = ADUC_GeneralResult_Failure,
                       .ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_CREATE_FAILURE_NO_SYMBOL };
            goto done;
        }
    }

    *contentDownloaderLibrary = _contentDownloader = extensionLib;

    result = { ADUC_Result_Success };

done:
    return result;
}

ADUC_Result ExtensionManager::SetContentDownloaderLibrary(void* contentDownloaderLibrary)
{
    ADUC_Result result = { ADUC_Result_Success };
    _contentDownloader = contentDownloaderLibrary;
    return result;
}

bool ExtensionManager::IsComponentsEnumeratorRegistered()
{
    void* extensionLib = nullptr;
    ADUC_Result result = LoadComponentEnumeratorLibrary(&extensionLib);
    return (IsAducResultCodeSuccess(result.ResultCode) && extensionLib != nullptr);
}

ADUC_Result ExtensionManager::LoadComponentEnumeratorLibrary(void** componentEnumerator)
{
    ADUC_Result result = { ADUC_Result_Failure };
    void* mainFunc = nullptr;
    void* extensionLib = nullptr;
    const char* requiredFunction = "GetAllComponents";

    if (_componentEnumerator != nullptr)
    {
        *componentEnumerator = _componentEnumerator;
        result = { ADUC_GeneralResult_Success };
        goto done;
    }

    result = LoadExtensionLibrary(
        "Component Enumerator",
        ADUC_EXTENSIONS_FOLDER,
        ADUC_EXTENSIONS_SUBDIR_COMPONENT_ENUMERATOR,
        ADUC_EXTENSION_REG_FILENAME,
        requiredFunction,
        ADUC_FACILITY_EXTENSION_COMPONENT_ENUMERATOR,
        0,
        &extensionLib);

    if (IsAducResultCodeFailure(result.ResultCode) || extensionLib == nullptr)
    {
        goto done;
    }

    dlerror(); // Clear any existing error
    try
    {
        mainFunc = dlsym(extensionLib, requiredFunction);
    }
    catch (...)
    {
    }

    if (mainFunc == nullptr)
    {
        // Log info instead of error since some extension is optional and may not be registered.
        Log_Info("The specified function ('%s') doesn't exist. %s\n", requiredFunction, dlerror());
        result = { .ResultCode = ADUC_GeneralResult_Failure,
                   .ExtendedResultCode = ADUC_ERC_UPDATE_CONTENT_HANDLER_CREATE_FAILURE_NO_SYMBOL };
        goto done;
    }

    *componentEnumerator = _componentEnumerator = extensionLib;

    result = { ADUC_Result_Success };

done:
    return result;
}

void ExtensionManager::_FreeComponentsDataString(char* componentsJson)
{
    void* lib = nullptr;
    FreeComponentsDataStringProc freeComponentsDataStringProc = nullptr;

    ADUC_Result result = ExtensionManager::LoadComponentEnumeratorLibrary(&lib);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    freeComponentsDataStringProc = reinterpret_cast<FreeComponentsDataStringProc>(
        dlsym(lib, "FreeComponentsDataString")); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

    if (freeComponentsDataStringProc == nullptr)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_COMPONENT_ENUMERATOR_FREECOMPONENTSDATASTRING_NOTIMP };

        goto done;
    }

    try
    {
        freeComponentsDataStringProc(componentsJson);
    }
    catch (...)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_COMPONENT_ENUMERATOR_EXCEPTION_FREECOMPONENTSDATASTRING };
        goto done;
    }

    result = { ADUC_GeneralResult_Success };

done:
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Warn("Cannot free a components data string. (extendedResult: 0x%X)", result.ExtendedResultCode);
    }
}

/**
 * @brief Returns all components information in JSON format.
 * @param[out] outputComponentsData An output string containing components data.
 */
ADUC_Result ExtensionManager::GetAllComponents(std::string& outputComponentsData)
{
    void* lib = nullptr;
    GetAllComponentsProc _getAllComponents = nullptr;
    char* components = nullptr;

    outputComponentsData = "";

    ADUC_Result result = ExtensionManager::LoadComponentEnumeratorLibrary(&lib);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    _getAllComponents = reinterpret_cast<GetAllComponentsProc>(dlsym(lib, "GetAllComponents"));
    if (_getAllComponents == nullptr)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_COMPONENT_ENUMERATOR_GETALLCOMPONENTS_NOTIMP };
        goto done;
    }

    try
    {
        components = _getAllComponents();
    }
    catch (...)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_COMPONENT_ENUMERATOR_EXCEPTION_GETALLCOMPONENTS };
        goto done;
    }

    if (components != nullptr)
    {
        outputComponentsData = components;
        _FreeComponentsDataString(components);
    }

    result = { ADUC_GeneralResult_Success };

done:
    return result;
}

/**
 * @brief Returns all components information in JSON format.
 * @param selector A JSON string contains name-value pairs used for selecting components.
 * @param[out] outputComponentsData An output string containing components data.
 */
ADUC_Result ExtensionManager::SelectComponents(const std::string& selector, std::string& outputComponentsData)
{
    void* lib = nullptr;
    SelectComponentsProc _selectComponents = nullptr;
    char* components = nullptr;

    outputComponentsData = "";

    ADUC_Result result = ExtensionManager::LoadComponentEnumeratorLibrary(&lib);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    _selectComponents = reinterpret_cast<SelectComponentsProc>(dlsym(lib, "SelectComponents"));
    if (_selectComponents == nullptr)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_COMPONENT_ENUMERATOR_SELECTCOMPONENTS_NOTIMP };
        goto done;
    }

    try
    {
        components = _selectComponents(selector.c_str());
    }
    catch (...)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_COMPONENT_ENUMERATOR_EXCEPTION_GETALLCOMPONENTS };
        goto done;
    }

    if (components != nullptr)
    {
        outputComponentsData = components;
        _FreeComponentsDataString(components);
    }

done:
    return result;
}

ADUC_Result ExtensionManager::InitializeContentDownloader(const char* initializeData)
{
    void* lib = nullptr;
    InitializeProc _initialize = nullptr;
    char* components = nullptr;

    ADUC_Result result = ExtensionManager::LoadContentDownloaderLibrary(&lib);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    _initialize = reinterpret_cast<InitializeProc>(dlsym(lib, "Initialize"));
    if (_initialize == nullptr)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_INITIALIZEPROC_NOTIMP };
        goto done;
    }

    try
    {
        result = _initialize(initializeData);
    }
    catch (...)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_INITIALIZE_EXCEPTION };
        goto done;
    }

done:
    return result;
}

ADUC_Result ExtensionManager::Download(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int retryTimeout,
    ADUC_DownloadProgressCallback downloadProgressCallback)
{
    void* lib = nullptr;
    DownloadProc downloadProc = nullptr;
    char* components = nullptr;
    SHAversion algVersion;
    std::stringstream childManifestFile;
    ADUC_Result result;

    try
    {
        childManifestFile << workFolder << "/" << entity->TargetFilename;
    }
    catch (...)
    {
        Log_Error("Cannot construct child manifest file path.");
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_BAD_CHILD_MANIFEST_FILE_PATH };
        goto done;
    }

    result = ExtensionManager::LoadContentDownloaderLibrary(&lib);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    downloadProc = reinterpret_cast<DownloadProc>(dlsym(lib, "Download"));
    if (downloadProc == nullptr)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_INITIALIZEPROC_NOTIMP };
        goto done;
    }

    if (!ADUC_HashUtils_GetShaVersionForTypeString(
            ADUC_HashUtils_GetHashType(entity->Hash, entity->HashCount, 0), &algVersion))
    {
        Log_Error(
            "FileEntity for %s has unsupported hash type %s",
            childManifestFile.str().c_str(),
            ADUC_HashUtils_GetHashType(entity->Hash, entity->HashCount, 0));
        result.ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_FILE_HASH_TYPE_NOT_SUPPORTED;
        goto done;
    }

    // If file exists and has a valid hash, then skip download.
    // Otherwise, delete an existing file, then download.
    Log_Debug("Check whether '%s' has already been download into the work folder.", childManifestFile.str().c_str());

    if (access(childManifestFile.str().c_str(), F_OK) == 0)
    {
        char* hashValue = ADUC_HashUtils_GetHashValue(entity->Hash, entity->HashCount, 0 /* index */);
        if (hashValue == nullptr)
        {
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_INVALID_FILE_ENTITY_NO_HASHES };
            goto done;
        }

        // If target file exists, validate file hash.
        // If file is valid, then skip the download.
        bool validHash = ADUC_HashUtils_IsValidFileHash(
            childManifestFile.str().c_str(), hashValue, algVersion, false /* suppressErrorLog */);

        if (!validHash)
        {
            // Delete existing file.
            if (remove(childManifestFile.str().c_str()) != 0)
            {
                Log_Error("Cannot delete existing file that has invalid hash.");
                result.ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_CANNOT_DELETE_EXISTING_FILE;
                goto done;
            }
        }

        result = { .ResultCode = ADUC_Result_Success, .ExtendedResultCode = 0 };
        goto done;
    }

    try
    {
        result = downloadProc(entity, workflowId, workFolder, retryTimeout, downloadProgressCallback);
    }
    catch (...)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_DOWNLOAD_EXCEPTION };
        goto done;
    }

    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        if (!ADUC_HashUtils_IsValidFileHash(
                childManifestFile.str().c_str(),
                ADUC_HashUtils_GetHashValue(entity->Hash, entity->HashCount, 0 /* index */),
                algVersion,
                true))
        {
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_DOWNLOAD_EXCEPTION };
            goto done;
        }
    }

done:
    return result;
}

EXTERN_C_BEGIN

ADUC_Result ExtensionManager_InitializeContentDownloader(const char* initializeData)
{
    return ExtensionManager::InitializeContentDownloader(initializeData);
}

ADUC_Result ExtensionManager_Download(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int retryTimeout,
    ADUC_DownloadProgressCallback downloadProgressCallback)
{
    return ExtensionManager::Download(entity, workflowId, workFolder, retryTimeout, downloadProgressCallback);
}

/**
 * @brief Uninitializes the extension manager.
 */
void ExtensionManager_Uninit()
{
    ExtensionManager::Uninit();
}

EXTERN_C_END
