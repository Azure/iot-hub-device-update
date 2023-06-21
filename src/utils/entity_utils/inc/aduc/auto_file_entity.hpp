/**
 * @file auto_file_entity.h
 * @brief Defines AutoFileEntity class that houses and manages an ADUC_FileEntity member.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef AUTO_FILE_ENTITY
#define AUTO_FILE_ENTITY

#include <aduc/parser_utils.h> // ADUC_FileEntity_Uninit
#include <aduc/types/update_content.h> // ADUC_FileEntity

struct AutoFileEntity : public ADUC_FileEntity
{
    ~AutoFileEntity()
    {
        ADUC_FileEntity_Uninit(this);
    }
};

#endif // #define AUTO_FILE_ENTITY
