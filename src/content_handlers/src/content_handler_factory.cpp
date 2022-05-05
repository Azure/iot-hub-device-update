/**
 * @file content_handler_factory.cpp
 * @brief Implementation of ContentHandlerFactory.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/content_handler_factory.hpp"
#include "aduc/content_handler.hpp"

#include "aduc/c_utils.h"
#include "aduc/exceptions.hpp"
#include "aduc/extension_utils.h"
#include "aduc/hash_utils.h" // for SHAversion
#include "aduc/logging.h"
#include "aduc/parser_utils.h"
#include "aduc/result.h"
#include "aduc/string_utils.hpp"

#include <cstring>
#include <unordered_map>
#include <vector>

// Note: this requires ${CMAKE_DL_LIBS}
#include <dlfcn.h>

std::unordered_map<std::string, void*> ContentHandlerFactory::_libs;
std::unordered_map<std::string, ContentHandler*> ContentHandlerFactory::_contentHandlers;

/**
 * @brief Loads UpdateContentHandler extension.
 * @param updateType An update type string.
 * @param libHandle A buffer for storing output extension library handle.
 * @return ADUC_Result contains result code and extended result code.
 */
ADUC_Result ContentHandlerFactory::LoadExtensionLibrary(const std::string& updateType, void** libHandle)
{
    ADUC_Result result{ ADUC_GeneralResult_Success };

    Log_Info("Loading Update Content Handler for '%s'.", updateType.c_str());

    if (libHandle == nullptr)
    {
        Log_Error("Invalid argument(s).");
        return { ADUC_GeneralResult_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_CREATE_FAILURE_INVALID_ARG };
    }

    // Try to find cached handle.
    try
    {
        *libHandle = ContentHandlerFactory::_libs.at(updateType);
        return { ADUC_GeneralResult_Success };
    }
    catch (...)
    {
        Log_Debug("Cache not found. Will create a new one.");
    }

    char* error = nullptr;
    UPDATE_CONTENT_HANDLER_CREATE_PROC createUpdateContentHandlerExtension = nullptr;
    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));

    if (!GetUpdateContentHandlerFileEntity(updateType.c_str(), &entity))
    {
        Log_Error("Update Content Handler for '%s' not found.", updateType.c_str());
        return { ADUC_GeneralResult_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_CREATE_FAILURE_NOT_FOUND };
    }

    // Validate file hash.
    SHAversion algVersion;
    if (!ADUC_HashUtils_GetShaVersionForTypeString(
            ADUC_HashUtils_GetHashType(entity.Hash, entity.HashCount, 0), &algVersion))
    {
        Log_Error(
            "FileEntity for %s has unsupported hash type %s",
            entity.TargetFilename,
            ADUC_HashUtils_GetHashType(entity.Hash, entity.HashCount, 0));
        result = { ADUC_GeneralResult_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_CREATE_FAILURE_VALIDATE };
        goto done;
    }

    if (!ADUC_HashUtils_IsValidFileHash(
            entity.TargetFilename,
            ADUC_HashUtils_GetHashValue(entity.Hash, entity.HashCount, 0),
            algVersion,
            false))
    {
        Log_Error("Hash for %s is not valid", entity.TargetFilename);
        result = { ADUC_GeneralResult_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_CREATE_FAILURE_VALIDATE };
        goto done;
    }

    Log_Debug("Loading update content handler from '%s'.", entity.TargetFilename);

    *libHandle = dlopen(entity.TargetFilename, RTLD_LAZY);

    if (*libHandle == nullptr)
    {
        Log_Error("Cannot load content handler file %s. %s.", entity.TargetFilename, dlerror());
        result = { ADUC_GeneralResult_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_CREATE_FAILURE_LOAD };
        goto done;
    }

    dlerror(); // Clear any existing error

    createUpdateContentHandlerExtension =
        reinterpret_cast<UPDATE_CONTENT_HANDLER_CREATE_PROC>(dlsym(*libHandle, "CreateUpdateContentHandlerExtension")); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

    if (createUpdateContentHandlerExtension == nullptr)
    {
        Log_Error("The specified function doesn't exist. %s\n", dlerror());
        result = { ADUC_GeneralResult_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_CREATE_FAILURE_NO_SYMBOL };
        goto done;
    }

    // Cache the loaded library.
    try
    {
        ContentHandlerFactory::_libs.emplace(updateType, *libHandle);
    }
    catch (...)
    {
        Log_Warn("Failed to cache lib handle.");
    }

done:
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        if (libHandle != nullptr)
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
ContentHandlerFactory::LoadUpdateContentHandlerExtension(const std::string& updateType, ContentHandler** handler)
{
    UPDATE_CONTENT_HANDLER_CREATE_PROC createUpdateContentHandlerExtension = nullptr;
    void* libHandle = nullptr;
    ADUC_Result result = { ADUC_GeneralResult_Success };

    Log_Info("Loading Update Content Handler for '%s'.", updateType.c_str());

    if (handler == nullptr)
    {
        Log_Error("Invalid argument(s).");
        return { ADUC_GeneralResult_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_CREATE_FAILURE_INVALID_ARG };
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

    result = LoadExtensionLibrary(updateType, &libHandle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    dlerror(); // Clear any existing error

    createUpdateContentHandlerExtension =
        reinterpret_cast<UPDATE_CONTENT_HANDLER_CREATE_PROC>(dlsym(libHandle, "CreateUpdateContentHandlerExtension")); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

    if (createUpdateContentHandlerExtension == nullptr)
    {
        Log_Error("The specified function doesn't exist. %s\n", dlerror());
        result = { ADUC_GeneralResult_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_CREATE_FAILURE_NO_SYMBOL };
        goto done;
    }

    try
    {
        *handler = createUpdateContentHandlerExtension(ADUC_Logging_GetLevel());
    }
    catch (const std::exception& ex)
    {
        Log_Error("An exception occurred while laoding content handler for '%s': %s", updateType.c_str(), ex.what());
    }
    catch (...)
    {
        Log_Error("An unknown exception occurred while laoding content handler for '%s'", updateType.c_str());
    }

    if (*handler == nullptr)
    {
        result = { ADUC_GeneralResult_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_CREATE_FAILURE_CREATE };
        goto done;
    }

    Log_Debug("Caching new content handler for '%s'.", updateType.c_str());
    _contentHandlers.emplace(updateType, *handler);

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

void ContentHandlerFactory::UnloadAllUpdateContentHandlers()
{
    for (auto& contentHandler : _contentHandlers)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc, hicpp-no-malloc)
        delete (contentHandler.second);
    }

    _contentHandlers.clear();
}

/**
 * @brief This API unloads all handlers then unload all extension libraries.
 *
 */
void ContentHandlerFactory::UnloadAllExtensions()
{
    // Make sure we unload every handlers first.
    UnloadAllUpdateContentHandlers();

    for (auto& lib : _libs)
    {
        dlclose(lib.second);
    }

    _libs.clear();
}
