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
#include <cstring> // memset

class AutoFileEntity
{
    ADUC_FileEntity entity{};

public:
    AutoFileEntity(const AutoFileEntity&) = delete;
    AutoFileEntity& operator=(const AutoFileEntity&) = delete;
    AutoFileEntity(AutoFileEntity&&) = delete;
    AutoFileEntity& operator=(AutoFileEntity&&) = delete;

    AutoFileEntity() = default;

    ~AutoFileEntity()
    {
        uninit();
    }

    const ADUC_FileEntity* operator->() const
    {
        return &entity;
    }

    /**
     * @brief Get the const addres of the file entity object
     * @return const ADUC_FileEntity* Address of wrapped object.
     */
    const ADUC_FileEntity* get() const
    {
        return &entity;
    }

    /**
     * @brief Get the address of the file entity object.
     * Useful for passing to methods with the signature "f(..., ADUC_FileEntity* outFileEntity)"
     *
     * @return ADUC_FileEntity* Non-const address of wrapped object.
     * @details Will uninitialize the current field as the assumption is the entity will be reinitialized by the caller.
     */
    ADUC_FileEntity* address_of()
    {
        uninit();
        return &entity;
    }

private:
    void uninit()
    {
        ADUC_FileEntity_Uninit(&entity);
    }
};

#endif // #define AUTO_FILE_ENTITY
