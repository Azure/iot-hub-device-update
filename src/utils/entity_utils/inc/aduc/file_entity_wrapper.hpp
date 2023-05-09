/**
 * @file file_entity_wrapper.h
 * @brief Defines FileEntityWrapper class, which performs RAII for ADUC_FileEntity
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef FILE_ENTITY_WRAPPER
#define FILE_ENTITY_WRAPPER

#include <aduc/parser_utils.h> // ADUC_FileEntity_Uninit
#include <aduc/types/update_content.h> // ADUC_FileEntity
#include <cstring> // memset

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

#endif // #define FILE_ENTITY_WRAPPER
