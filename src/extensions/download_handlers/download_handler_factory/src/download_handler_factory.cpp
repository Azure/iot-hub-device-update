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
#include <aduc/auto_file_entity.hpp> // AutoFileEntity
#include <aduc/hash_utils.h> // ADUC_HashUtils_VerifyWithStrongestHash
#include <aduc/logging.h> // ADUC_Logging_GetLevel
#include <aduc/plugin_exception.hpp>
#include <aduc/types/update_content.h> // ADUC_FileEntity
#include <unordered_map>

using DownloadHandlerHandle = void*;

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

    AutoFileEntity downloadHandlerFileEntity;
    if (!GetDownloadHandlerFileEntity(downloadHandlerId.c_str(), &downloadHandlerFileEntity))
    {
        Log_Error("Failed to get DownloadHandler for file entity");
        return nullptr;
    }


    if (!ADUC_HashUtils_VerifyWithStrongestHash(
            downloadHandlerFileEntity.TargetFilename, downloadHandlerFileEntity.Hash, downloadHandlerFileEntity.HashCount))
    {
        Log_Error("verify hash failed for %s", downloadHandlerFileEntity.TargetFilename);
        return nullptr;
    }

    try
    {
        auto plugin = std::unique_ptr<DownloadHandlerPlugin>(
            new DownloadHandlerPlugin(downloadHandlerFileEntity.TargetFilename, ADUC_Logging_GetLevel()));
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
