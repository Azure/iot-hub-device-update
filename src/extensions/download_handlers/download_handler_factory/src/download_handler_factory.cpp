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
#include <aduc/plugin_exception.hpp>
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

DownloadHandlerPlugin* DownloadHandlerFactory::LoadDownloadHandler(const std::string& downloadHandlerId) noexcept
{
    auto entry = cachedPlugins.find(downloadHandlerId);
    if (entry != cachedPlugins.end())
    {
        Log_Debug("Found cached plugin for id %s", downloadHandlerId.c_str());
        return (entry->second).get();
    }

    ADUC_FileEntity downloadHandlerFileEntity = {};
    if (!GetDownloadHandlerFileEntity(downloadHandlerId.c_str(), &downloadHandlerFileEntity))
    {
        Log_Error("Failed to get DownloadHandler for file entity");
        return nullptr;
    }

    FileEntityWrapper autoFileEntity(&downloadHandlerFileEntity);

    if (!ADUC_HashUtils_VerifyWithStrongestHash(
            autoFileEntity->TargetFilename, autoFileEntity->Hash, autoFileEntity->HashCount))
    {
        Log_Error("verify hash failed for %s", autoFileEntity->TargetFilename);
        return nullptr;
    }

    try
    {
        auto plugin = std::unique_ptr<DownloadHandlerPlugin>(
            new DownloadHandlerPlugin(autoFileEntity->TargetFilename, ADUC_Logging_GetLevel()));
        cachedPlugins.insert(std::make_pair(downloadHandlerId, plugin.get()));
        return plugin.release();
    }
    catch (const aduc::PluginException& pe)
    {
        Log_Error("plugin exception: %s, sym: %s", pe.what(), pe.Symbol().c_str());
        return nullptr;
    }
    catch (const std::exception& e)
    {
        Log_Error("exception: %s", e.what());
        return nullptr;
    }
    catch (...)
    {
        Log_Error("non std exception");
        return nullptr;
    }
}

EXTERN_C_BEGIN

DownloadHandlerHandle ADUC_DownloadHandlerFactory_LoadDownloadHandler(const char* downloadHandlerId)
{
    DownloadHandlerFactory* factory = DownloadHandlerFactory::GetInstance();
    DownloadHandlerPlugin* plugin = factory->LoadDownloadHandler(downloadHandlerId);
    return reinterpret_cast<DownloadHandlerHandle>(plugin);
}

EXTERN_C_END
