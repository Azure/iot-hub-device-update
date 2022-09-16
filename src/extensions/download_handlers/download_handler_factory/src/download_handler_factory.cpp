/**
 * @file download_handler_factory.cpp
 * @brief Implementation of the DownloadHandlerFactory.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/download_handler_factory.hpp"
#include "aduc/download_handler_plugin.hpp" // DownloadHandlerPlugin
#include <aduc/c_utils.h> // EXTERN_C_{BEGIN,END}
#include <aduc/extension_utils.h> // GetDownloadHandlerFileEntity
#include <aduc/hash_utils.h> // ADUC_HashUtils_VerifyWithStrongestHash
#include <aduc/logging.h> // ADUC_Logging_GetLevel
#include <aduc/parser_utils.h> // ADUC_FileEntity_Uninit
#include <aduc/types/update_content.h> // ADUC_FileEntity
#include <cstring> // memset
#include <unordered_map>

using DownloadHandlerHandle = void*;

class FileEntityWrapper
{
    ADUC_FileEntity entity = {};
    bool inited = false;

public:
    FileEntityWrapper() = delete;
    FileEntityWrapper(const FileEntityWrapper&) = delete;
    FileEntityWrapper& operator=(const FileEntityWrapper&) = delete;
    FileEntityWrapper(FileEntityWrapper&&) = delete;
    FileEntityWrapper& operator=(FileEntityWrapper&&) = delete;

    FileEntityWrapper(ADUC_FileEntity* sourceFileEntity)
    {
        // transfer ownership
        entity = *sourceFileEntity;
        memset(sourceFileEntity, 0, sizeof(ADUC_FileEntity));
        inited = true;
    }

    ~FileEntityWrapper()
    {
        if (inited)
        {
            inited = false;
            ADUC_FileEntity_Uninit(&entity);
        }
    }

    const ADUC_FileEntity* operator->()
    {
        return &entity;
    }
};

// static
DownloadHandlerFactory* DownloadHandlerFactory::GetInstance()
{
    static DownloadHandlerFactory s_instance;
    return &s_instance;
}

DownloadHandlerPlugin* DownloadHandlerFactory::LoadDownloadHandler(const std::string& downloadHandlerId)
{
    auto entry = cachedPlugins.find(downloadHandlerId);
    if (entry != cachedPlugins.end())
    {
        return (entry->second).get();
    }

    ADUC_FileEntity downloadHandlerFileEntity = {};
    if (!GetDownloadHandlerFileEntity(downloadHandlerId.c_str(), &downloadHandlerFileEntity))
    {
        return nullptr;
    }

    FileEntityWrapper autoFileEntity(&downloadHandlerFileEntity);

    if (!ADUC_HashUtils_VerifyWithStrongestHash(
            autoFileEntity->TargetFilename, autoFileEntity->Hash, autoFileEntity->HashCount))
    {
        return nullptr;
    }

    try
    {
        auto plugin = std::unique_ptr<DownloadHandlerPlugin>(
            new DownloadHandlerPlugin(autoFileEntity->TargetFilename, ADUC_Logging_GetLevel()));
        cachedPlugins.insert(std::make_pair(downloadHandlerId, plugin.get()));
        return plugin.release();
    }
    catch (...)
    {
        return nullptr;
    }
}

EXTERN_C_BEGIN

DownloadHandlerHandle ADUC_DownloadHandlerFactory_LoadDownloadHandler(const char* downloadHandlerId)
{
    DownloadHandlerHandle handle = nullptr;

    try
    {
        DownloadHandlerFactory* factory = DownloadHandlerFactory::GetInstance();
        DownloadHandlerPlugin* plugin = factory->LoadDownloadHandler(downloadHandlerId);
        handle = reinterpret_cast<DownloadHandlerHandle>(plugin);
    }
    catch (...)
    {
    }

    return handle;
}

EXTERN_C_END
