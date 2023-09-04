/**
 * @file extension_manager_modules.cpp
 * @brief Implementation of ExtensionManager.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/extension_manager.h"

#include <aduc/c_utils.h>
#include <aduc/calloc_wrapper.hpp> // ADUC::StringUtils::cstr_wrapper
#include <aduc/component_enumerator_extension.hpp>
#include <aduc/config_utils.h>
#include <aduc/content_downloader_extension.hpp>
#include <aduc/content_handler.hpp>
#include <aduc/contract_utils.h>
#include <aduc/exceptions.hpp>
#include <aduc/exports/extension_export_symbols.h>
#include <aduc/extension_manager.hpp>
#include <aduc/extension_manager_helper.hpp>
#include <aduc/extension_utils.h>
#include <aduc/hash_utils.h> // for SHAversion
#include <aduc/logging.h>
#include <aduc/parser_utils.h>
#include <aduc/path_utils.h> // SanitizePathSegment
#include <aduc/plugin_exception.hpp>
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

#include <dirent.h> // for opendir, readdir, closedir, DIR

#define ADUC_AGENT_MODULE_REGISTRY_FOLDER "var/lib/adu/extensions/modules" //!< Folder where agent modules are registered.
#define ADUC_AGENT_MODULE_REGISTRY_FILENAME ADUC_EXTENSION_REG_FILENAME //!< Filename of the module registration file.
#define ADUC_MAX_AGENT_MODULE 100 //!< Max number of agent modules that can be registered.
#define PATH_MAX 1024 //!< Max path length.
#define AGENT_MODULE__GetAllComponents__EXPORT_SYMBOL

/**
 * @brief Folder names comparison function for qsort
 *
 * @param a First folder name
 * @param b Second folder name
 * @return int 0 if equal, -1 if a < b, 1 if a > b.
 */
static int compare_folder_name(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

/**
 * @brief Load an agent module from the specified @p folder.
 *
 * @param requiredFunction The name of the required function to check for.
 * @param facilityCode Facility code for the error code.
 * @param componentCode Component code for the error code.
 * @param libHandle Pointer to the handle of the loaded library.
 * @return ADUC_Result The result of the operation.
 */
static ADUC_Result load_agent_module(const char *folder) {
    const char* requiredFunction,
    int facilityCode,
    int componentCode,
    void** libHandle)
{
    ADUC_Result result{ ADUC_GeneralResult_Failure };
    ADUC_FileEntity entity = {};
    SHAversion algVersion;

    std::stringstream path;
    path << folder << "/" << ADUC_AGENT_MODULE_REGISTRY_FILENAME;

    Log_Info("Loading extension '%s'. Reg file : %s", extensionName, path.str().c_str());

    if (libHandle == nullptr)
    {
        Log_Error("Invalid argument(s).");
        result.ExtendedResultCode = ADUC_ERC_EXTENSION_CREATE_FAILURE_INVALID_ARG(facilityCode, componentCode);
        goto done;
    }

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
        Log_Error("Cannot load handler file %s. %s.", entity.TargetFilename, dlerror());
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

ADUC_Result ExtensionManager::LoadAgentModules()
{
    ADUC_Result result{ ADUC_GeneralResult_Failure };

    struct dirent *entry;
    DIR *dir = opendir(ADUC_AGENT_MODULE_REGISTRY_FOLDER);

    if (dir == NULL) {
        Log_Error("Error opening directory");
        // TODO: Set ADUC_Result
        goto done;
    }

    char *folders[ADUC_MAX_AGENT_MODULE];  // Assuming max 100 extensions for simplicity
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        // Check if the entry is a directory
        struct stat info;
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s",ADUC_AGENT_MODULE_REGISTRY_FOLDER, entry->d_name);
        if (stat(path, &info) != -1 && S_ISDIR(info.st_mode)) {
            if (entry->d_name[0] != '.') {  // Ignore . and ..
                folders[count++] = strdup(entry->d_name);
            }
        }
    }
    closedir(dir);

    // Sort the folders
    qsort(folders, count, sizeof(char *), compare_folder_name);

    // Load extensions from the sorted folders
    for (int i = 0; i < count; i++) {
        if (IsAducResultCodeSuccess(result))
        {
            result = load_agent_module(folders[i]);
            if (!IsAducResultCodeSuccess(result)) {
                Log_Error("Error loading module from: %s", folders[i]);
            }
        }
        free(folders[i]);
        folders[i] = nullptr;
    }

done:
    return result;
}

ADUC_Result ExtensionManager::UnloadApplicationModules()
{
    return ADUC_Result{ ADUC_GeneralResult_Success };
}
