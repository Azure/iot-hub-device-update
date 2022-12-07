/**
 * @file extension_manager.cpp
 * @brief Implementation of ExtensionManager.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/component_enumerator_extension.hpp>
#include <aduc/content_downloader_extension.hpp>
#include <aduc/content_handler.hpp>
#include <aduc/download_handler_factory.hpp>
#include <aduc/download_handler_plugin.hpp>

#include <aduc/c_utils.h>
#include <aduc/calloc_wrapper.hpp> // ADUC::StringUtils::cstr_wrapper
#include <aduc/contract_utils.h>
#include <aduc/exceptions.hpp>
#include <aduc/exports/extension_export_symbols.h>
#include <aduc/extension_manager.h>
#include <aduc/extension_manager.hpp>
#include <aduc/extension_utils.h>
#include <aduc/hash_utils.h> // for SHAversion
#include <aduc/logging.h>
#include <aduc/parser_utils.h>
#include <aduc/path_utils.h> // SanitizePathSegment
#include <aduc/result.h>
#include <aduc/string_c_utils.h>
#include <aduc/string_handle_wrapper.hpp>
#include <aduc/string_utils.hpp>
#include <aduc/types/workflow.h> // ADUC_WorkflowHandle
#include <aduc/workflow_utils.h>

#include <cstring>
#include <unordered_map>

// Note: this requires ${CMAKE_DL_LIBS}
#include <dlfcn.h>
#include <unistd.h>

// type aliases
using UPDATE_CONTENT_HANDLER_CREATE_PROC = ContentHandler* (*)(ADUC_LOG_SEVERITY logLevel);
using GET_CONTRACT_INFO_PROC = ADUC_Result (*)(ADUC_ExtensionContractInfo* contractInfo);
using WorkflowHandle = void*;
using ADUC::StringUtils::cstr_wrapper;

EXTERN_C_BEGIN
ExtensionManager_Download_Options Default_ExtensionManager_Download_Options = {
    .retryTimeout = 60 * 60 * 24 /* default : 24 hour */,
};
EXTERN_C_END

// Static members.
std::unordered_map<std::string, void*> ExtensionManager::_libs;
std::unordered_map<std::string, ContentHandler*> ExtensionManager::_contentHandlers;
void* ExtensionManager::_contentDownloader;
ADUC_ExtensionContractInfo ExtensionManager::_contentDownloaderContractVersion;
void* ExtensionManager::_componentEnumerator;
ADUC_ExtensionContractInfo ExtensionManager::_componentEnumeratorContractVersion;

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

    UPDATE_CONTENT_HANDLER_CREATE_PROC createUpdateContentHandlerExtensionFn = nullptr;
    GET_CONTRACT_INFO_PROC getContractInfoFn = nullptr;
    void* libHandle = nullptr;
    ADUC_ExtensionContractInfo contractInfo{};

    Log_Info("Loading Update Content Handler for '%s'.", updateType.c_str());

    if (handler == nullptr)
    {
        Log_Error("Invalid argument(s).");
        result.ExtendedResultCode =
            ADUC_ERC_EXTENSION_CREATE_FAILURE_INVALID_ARG(ADUC_FACILITY_EXTENSION_UPDATE_CONTENT_HANDLER, 0);
        return result;
    }

    ADUC::StringUtils::STRING_HANDLE_wrapper folderName{ SanitizePathSegment(updateType.c_str()) };
    if (folderName.is_null())
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        return result;
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

    result = LoadExtensionLibrary(
        updateType.c_str(),
        ADUC_UPDATE_CONTENT_HANDLER_EXTENSION_DIR,
        folderName.c_str(),
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
    createUpdateContentHandlerExtensionFn = reinterpret_cast<UPDATE_CONTENT_HANDLER_CREATE_PROC>(
        dlsym(libHandle, CONTENT_HANDLER__CreateUpdateContentHandlerExtension__EXPORT_SYMBOL));

    if (createUpdateContentHandlerExtensionFn == nullptr)
    {
        Log_Error("The specified function doesn't exist. %s\n", dlerror());
        result.ExtendedResultCode =
            ADUC_ERC_EXTENSION_FAILURE_REQUIRED_FUNCTION_NOTIMPL(ADUC_FACILITY_EXTENSION_UPDATE_CONTENT_HANDLER, 0);
        goto done;
    }

    try
    {
        *handler = createUpdateContentHandlerExtensionFn(ADUC_Logging_GetLevel());
    }
    catch (const std::exception& ex)
    {
        Log_Error("An exception occurred while creating update handler: %s", ex.what());
    }
    catch (...)
    {
        Log_Error("Unknown exception occurred while creating update handler for '%s'", updateType.c_str());
    }

    if (*handler == nullptr)
    {
        result = { ADUC_GeneralResult_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_CREATE_FAILURE_CREATE };
        goto done;
    }

    Log_Debug("Determining contract version for '%s'.", updateType.c_str());

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    getContractInfoFn =
        reinterpret_cast<GET_CONTRACT_INFO_PROC>(dlsym(libHandle, CONTENT_HANDLER__GetContractInfo__EXPORT_SYMBOL));

    if (getContractInfoFn == nullptr)
    {
        Log_Info(
            "No '" CONTENT_HANDLER__GetContractInfo__EXPORT_SYMBOL "' symbol for '%s'. Defaulting V1.0",
            updateType.c_str());

        contractInfo.majorVer = ADUC_V1_CONTRACT_MAJOR_VER;
        contractInfo.minorVer = ADUC_V1_CONTRACT_MINOR_VER;
    }
    else
    {
        result = getContractInfoFn(&contractInfo);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            Log_Error(
                "'%s' extension call ERC: %08x",
                CONTENT_HANDLER__GetContractInfo__EXPORT_SYMBOL,
                result.ExtendedResultCode);
            result.ExtendedResultCode = ADUC_ERC_UPDATE_CONTENT_HANDLER_GET_CONTRACT_INFO_CALL_FAILURE;
            goto done;
        }

        Log_Debug(
            "Got %d.%d contract version for '%s' content handler",
            contractInfo.majorVer,
            contractInfo.minorVer,
            updateType.c_str());
    }

    (*handler)->SetContractInfo(contractInfo);

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
    static const char* functionNames[] = { CONTENT_DOWNLOADER__Initialize__EXPORT_SYMBOL,
                                           CONTENT_DOWNLOADER__Download__EXPORT_SYMBOL };
    void* extensionLib = nullptr;
    GET_CONTRACT_INFO_PROC getContractInfoFn = nullptr;

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

    Log_Debug("Determining contract version for content downloader.");

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    getContractInfoFn = reinterpret_cast<GET_CONTRACT_INFO_PROC>(
        dlsym(extensionLib, CONTENT_DOWNLOADER__GetContractInfo__EXPORT_SYMBOL));
    if (getContractInfoFn == nullptr)
    {
        _contentDownloaderContractVersion.majorVer = ADUC_V1_CONTRACT_MAJOR_VER;
        _contentDownloaderContractVersion.minorVer = ADUC_V1_CONTRACT_MINOR_VER;
        Log_Debug("No " CONTENT_DOWNLOADER__GetContractInfo__EXPORT_SYMBOL
                  "export. Defaulting to V1 contract for content downloader");
    }
    else
    {
        result = getContractInfoFn(&_contentDownloaderContractVersion);
        Log_Debug(
            "Got Contract %d.%d for content downloader",
            _contentDownloaderContractVersion.majorVer,
            _contentDownloaderContractVersion.minorVer);
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

ADUC_Result ExtensionManager::GetContentDownloaderContractVersion(ADUC_ExtensionContractInfo* contractInfo)
{
    *contractInfo = _contentDownloaderContractVersion;
    return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
}

ADUC_Result ExtensionManager::GetComponentEnumeratorContractVersion(ADUC_ExtensionContractInfo* contractInfo)
{
    ADUC_Result result = { ADUC_Result_Success };
    *contractInfo = _componentEnumeratorContractVersion;
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
    const char* requiredFunction = COMPONENT_ENUMERATOR__GetAllComponents__EXPORT_SYMBOL;
    GET_CONTRACT_INFO_PROC getContractInfoFn = nullptr;

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

    Log_Debug("Determining contract version for component enumerator.");

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    getContractInfoFn = reinterpret_cast<GET_CONTRACT_INFO_PROC>(
        dlsym(extensionLib, COMPONENT_ENUMERATOR__GetContractInfo__EXPORT_SYMBOL));
    if (getContractInfoFn == nullptr)
    {
        _componentEnumeratorContractVersion.majorVer = ADUC_V1_CONTRACT_MAJOR_VER;
        _componentEnumeratorContractVersion.minorVer = ADUC_V1_CONTRACT_MINOR_VER;
        Log_Debug("default to V1 contract for component enumerator");
    }
    else
    {
        getContractInfoFn(&_componentEnumeratorContractVersion);
        Log_Debug(
            "contract %d.%d for component enumerator",
            _componentEnumeratorContractVersion.majorVer,
            _componentEnumeratorContractVersion.minorVer);
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

    if (ExtensionManager::_componentEnumeratorContractVersion.majorVer == ADUC_V1_CONTRACT_MAJOR_VER
        && ExtensionManager::_componentEnumeratorContractVersion.minorVer == ADUC_V1_CONTRACT_MINOR_VER)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        freeComponentsDataStringProc = reinterpret_cast<FreeComponentsDataStringProc>(dlsym(
            lib,
            COMPONENT_ENUMERATOR__FreeComponentsDataString__EXPORT_SYMBOL)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

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
    }
    else
    {
        Log_Error(
            "Unsupported contract %d.%d",
            ExtensionManager::_componentEnumeratorContractVersion.majorVer,
            ExtensionManager::_componentEnumeratorContractVersion.minorVer);
        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = ADUC_ERC_COMPONENT_ENUMERATOR_UNSUPPORTED_CONTRACT_VERSION;
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
    static GetAllComponentsProc _getAllComponents = nullptr;

    void* lib = nullptr;
    char* components = nullptr;

    outputComponentsData = "";

    ADUC_Result result = ExtensionManager::LoadComponentEnumeratorLibrary(&lib);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    if (ExtensionManager::_componentEnumeratorContractVersion.majorVer == ADUC_V1_CONTRACT_MAJOR_VER
        && ExtensionManager::_componentEnumeratorContractVersion.minorVer == ADUC_V1_CONTRACT_MINOR_VER)
    {
        if (_getAllComponents == nullptr)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            _getAllComponents = reinterpret_cast<GetAllComponentsProc>(
                dlsym(lib, COMPONENT_ENUMERATOR__GetAllComponents__EXPORT_SYMBOL));
            if (_getAllComponents == nullptr)
            {
                result = { .ResultCode = ADUC_Result_Failure,
                           .ExtendedResultCode = ADUC_ERC_COMPONENT_ENUMERATOR_GETALLCOMPONENTS_NOTIMP };
                goto done;
            }
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
    }
    else
    {
        Log_Error(
            "Unsupported contract version %d.%d",
            ExtensionManager::_componentEnumeratorContractVersion.majorVer,
            ExtensionManager::_componentEnumeratorContractVersion.minorVer);
        result.ResultCode = ADUC_GeneralResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_COMPONENT_ENUMERATOR_UNSUPPORTED_CONTRACT_VERSION;
        goto done;
    }

    result = { ADUC_GeneralResult_Success, 0 };

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
    _selectComponents =
        reinterpret_cast<SelectComponentsProc>(dlsym(lib, COMPONENT_ENUMERATOR__SelectComponents__EXPORT_SYMBOL));
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

    if (ExtensionManager::_contentDownloaderContractVersion.majorVer != ADUC_V1_CONTRACT_MAJOR_VER
        && ExtensionManager::_contentDownloaderContractVersion.minorVer != ADUC_V1_CONTRACT_MINOR_VER)
    {
        Log_Error(
            "Unsupported contract version %d.%d",
            ExtensionManager::_contentDownloaderContractVersion.majorVer,
            ExtensionManager::_contentDownloaderContractVersion.minorVer);
        result.ResultCode = ADUC_GeneralResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_UNSUPPORTED_CONTRACT_VERSION;
        goto done;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    _initialize = reinterpret_cast<InitializeProc>(dlsym(lib, CONTENT_DOWNLOADER__Initialize__EXPORT_SYMBOL));
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
    WorkflowHandle workflowHandle,
    ExtensionManager_Download_Options* options,
    ADUC_DownloadProgressCallback downloadProgressCallback)
{
    void* lib = nullptr;
    DownloadProc downloadProc = nullptr;
    char* components = nullptr;
    SHAversion algVersion;

    ADUC_Result result = { .ResultCode = ADUC_Result_Failure, .ExtendedResultCode = 0 };
    ADUC::StringUtils::STRING_HANDLE_wrapper targetUpdateFilePath{ nullptr };

    if (!workflow_get_entity_workfolder_filepath(workflowHandle, entity, targetUpdateFilePath.address_of()))
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

    if (ExtensionManager::_contentDownloaderContractVersion.majorVer != ADUC_V1_CONTRACT_MAJOR_VER
        && ExtensionManager::_contentDownloaderContractVersion.minorVer != ADUC_V1_CONTRACT_MINOR_VER)
    {
        Log_Error(
            "Unsupported contract version %d.%d",
            ExtensionManager::_contentDownloaderContractVersion.majorVer,
            ExtensionManager::_contentDownloaderContractVersion.minorVer);
        result.ResultCode = ADUC_GeneralResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_UNSUPPORTED_CONTRACT_VERSION;
        goto done;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    downloadProc = reinterpret_cast<DownloadProc>(dlsym(lib, CONTENT_DOWNLOADER__Download__EXPORT_SYMBOL));
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
            targetUpdateFilePath.c_str(),
            ADUC_HashUtils_GetHashType(entity->Hash, entity->HashCount, 0));
        result.ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_FILE_HASH_TYPE_NOT_SUPPORTED;
        goto done;
    }

    // If file exists and has a valid hash, then skip download.
    // Otherwise, delete an existing file, then download.
    Log_Debug("Check whether '%s' has already been download into the work folder.", targetUpdateFilePath.c_str());

    if (access(targetUpdateFilePath.c_str(), F_OK) == 0)
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
            targetUpdateFilePath.c_str(), hashValue, algVersion, false /* suppressErrorLog */);

        if (!validHash)
        {
            // Delete existing file.
            if (remove(targetUpdateFilePath.c_str()) != 0)
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
        const char* workflowId = workflow_peek_id(workflowHandle);
        cstr_wrapper workFolder{ workflow_get_workfolder(workflowHandle) };

        result = { .ResultCode = ADUC_Result_Failure, .ExtendedResultCode = 0 };

        // First, attempt to produce the update using download handler if
        // download handler exists in the entity (metadata).
        if (!IsNullOrEmpty(entity->DownloadHandlerId))
        {
            DownloadHandlerFactory* factory = DownloadHandlerFactory::GetInstance();
            DownloadHandlerPlugin* plugin = factory->LoadDownloadHandler(entity->DownloadHandlerId);
            if (plugin == nullptr)
            {
                Log_Warn("Load Download Handler %s failed", entity->DownloadHandlerId);

                workflow_set_success_erc(
                    workflowHandle, ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_CREATE_FAILURE_CREATE);
            }
            else
            {
                ADUC_ExtensionContractInfo contractInfo{};

                Log_Debug("Getting contract info for download handler '%s'.", entity->DownloadHandlerId);

                result = plugin->GetContractInfo(&contractInfo);

                if (IsAducResultCodeFailure(result.ResultCode))
                {
                    Log_Error(
                        "GetContractInfo failed for download handler '%s': result 0x%08x, erc 0x%08x",
                        entity->DownloadHandlerId,
                        result.ResultCode,
                        result.ExtendedResultCode);
                    goto done;
                }

                Log_Debug(
                    "Downloadhandler '%s' Contract Version: %d.%d",
                    entity->DownloadHandlerId,
                    contractInfo.majorVer,
                    contractInfo.minorVer);

                if (contractInfo.majorVer == ADUC_V1_CONTRACT_MAJOR_VER
                    && contractInfo.minorVer == ADUC_V1_CONTRACT_MINOR_VER)
                {
                    result = plugin->ProcessUpdate(workflowHandle, entity, targetUpdateFilePath.c_str());
                    if (IsAducResultCodeFailure(result.ResultCode))
                    {
                        Log_Warn(
                            "Download handler failed to produce update: result 0x%08x, erc 0x%08x",
                            result.ResultCode,
                            result.ExtendedResultCode);

                        workflow_set_success_erc(workflowHandle, result.ExtendedResultCode);
                    }
                }
                else
                {
                    Log_Error("Unsupported contract %d.%d", contractInfo.majorVer, contractInfo.minorVer);

                    result.ResultCode = ADUC_GeneralResult_Failure;
                    result.ExtendedResultCode =
                        ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_UNSUPPORTED_CONTRACT_VERSION;

                    goto done;
                }
            }
        }

        // If no download handlers specified, or download handler failed to produce the target file.
        if (IsAducResultCodeFailure(result.ResultCode)
            || result.ResultCode == ADUC_Result_Download_Handler_RequiredFullDownload)
        {
            // Either download handler id did not exist, or download handler failed and doing fallback here.
            result =
                downloadProc(entity, workflowId, workFolder.get(), options->retryTimeout, downloadProgressCallback);
        }
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
                targetUpdateFilePath.c_str(),
                ADUC_HashUtils_GetHashValue(entity->Hash, entity->HashCount, 0),
                algVersion,
                false))
        {
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_INVALID_FILE_HASH };

            workflow_set_success_erc(workflowHandle, result.ExtendedResultCode);

            goto done;
        }
    }

    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;

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
    ADUC_WorkflowHandle workflowHandle,
    ExtensionManager_Download_Options* options,
    ADUC_DownloadProgressCallback downloadProgressCallback)
{
    return ExtensionManager::Download(entity, workflowHandle, options, downloadProgressCallback);
}

/**
 * @brief Uninitializes the extension manager.
 */
void ExtensionManager_Uninit()
{
    ExtensionManager::Uninit();
}

EXTERN_C_END
