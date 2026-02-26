/**
 * @file mock_factory_deps.cpp
 * @brief Link-time mock implementations for download_handler_factory dependencies.
 *
 * Provides mock implementations for:
 *   - GetDownloadHandlerFileEntity (from extension_utils)
 *   - ADUC_HashUtils_VerifyWithStrongestHash (from hash_utils)
 *   - ADUC_FileEntity_Uninit (from parser_utils)
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "mock_factory_deps.h"
#include <aduc/types/hash.h>
#include <aduc/types/update_content.h>

#include <cstring>

MockFactoryDepsState g_mockFactoryDeps{};

extern "C" {

bool GetDownloadHandlerFileEntity(const char* /* downloadHandlerId */, ADUC_FileEntity* fileEntity)
{
    if (!g_mockFactoryDeps.getFileEntityResult)
    {
        return false;
    }

    memset(fileEntity, 0, sizeof(ADUC_FileEntity));

    if (g_mockFactoryDeps.fileEntityPath != nullptr)
    {
        fileEntity->TargetFilename = strdup(g_mockFactoryDeps.fileEntityPath);
    }

    return true;
}

bool ADUC_HashUtils_VerifyWithStrongestHash(
    const char* /* filePath */, const ADUC_Hash* /* hashes */, size_t /* hashCount */)
{
    return g_mockFactoryDeps.verifyHashResult;
}

void ADUC_FileEntity_Uninit(ADUC_FileEntity* /* entity */)
{
    // No-op mock — prevent real parser_utils from freeing our strdup'd memory.
}

} // extern "C"
