/**
 * @file adu_types.c
 * @brief Implements helper functions related to ConnectionInfo
 * @copyright Copyright (c) Microsoft Corporation.
 */

#include "aduc/adu_types.h"
#include "aduc/connection_string_utils.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h"
#include "aduc/types/update_content.h"

#include "parson.h"

#include <stdlib.h>
#include <string.h>

#include <azure_c_shared_utility/crt_abstractions.h> // for mallocAndStrcpy_s

/**
 * @brief Returns the string associated with @p connType
 * @param connType ADUC_ConnType to be stringified
 * @returns if the ADUC_ConnType exists then the string version of the value is returned, "" otherwise
 */
const char* ADUC_ConnType_ToString(const ADUC_ConnType connType)
{
    switch (connType)
    {
    case ADUC_ConnType_NotSet:
        return "ADUC_ConnType_NotSet";
    case ADUC_ConnType_Device:
        return "ADUC_ConnType_Device";
    case ADUC_ConnType_Module:
        return "ADUC_ConnType_Module";
    }

    return "<Unknown>";
}
/**
 * @brief DeAllocates the ADUC_ConnectionInfo object
 * @param info the ADUC_ConnectionInfo object to be de-allocated
 */
void ADUC_ConnectionInfo_DeAlloc(ADUC_ConnectionInfo* info)
{
    free(info->connectionString);
    info->connectionString = NULL;

    free(info->certificateString);
    info->certificateString = NULL;

    free(info->opensslEngine);
    info->opensslEngine = NULL;

    free(info->opensslPrivateKey);
    info->opensslPrivateKey = NULL;

    info->authType = ADUC_AuthType_NotSet;
    info->connType = ADUC_ConnType_NotSet;
}

/**
 * @brief Scans the connection string and returns the connection type related to the string
 * @details The connection string must use the valid, correct format for the DeviceId and/or the ModuleId
 * e.g.
 * "DeviceId=some-device-id;ModuleId=some-module-id;"
 * If the connection string contains the DeviceId it is an ADUC_ConnType_Device
 * If the connection string contains the DeviceId AND the ModuleId it is an ADUC_ConnType_Module
 * @param connectionString the connection string to scan
 * @returns the connection type for @p connectionString
 */
ADUC_ConnType GetConnTypeFromConnectionString(const char* connectionString)
{
    ADUC_ConnType result = ADUC_ConnType_NotSet;

    if (connectionString == NULL)
    {
        Log_Debug("Connection string passed to GetConnTypeFromConnectionString is NULL");
        return ADUC_ConnType_NotSet;
    }

    if (ConnectionStringUtils_DoesKeyExist(connectionString, "DeviceId"))
    {
        if (ConnectionStringUtils_DoesKeyExist(connectionString, "ModuleId"))
        {
            result = ADUC_ConnType_Module;
        }
        else
        {
            result = ADUC_ConnType_Device;
        }
    }
    else
    {
        Log_Debug("DeviceId not present in connection string.");
    }

    return result;
}

/**
 * @brief Checks if the UpdateId is valid
 * @param updateId updateId to check
 * @returns True if it is valid, false if not
 */
_Bool ADUC_IsValidUpdateId(const ADUC_UpdateId* updateId)
{
    return !(
        updateId == NULL || IsNullOrEmpty(updateId->Provider) || IsNullOrEmpty(updateId->Name)
        || IsNullOrEmpty(updateId->Version));
}

/**
 * @brief Free the UpdateId and its content.
 * @param updateid a pointer to an updateId struct to be freed
 */
void ADUC_UpdateId_UninitAndFree(ADUC_UpdateId* updateId)
{
    if (updateId == NULL)
    {
        return;
    }

    free(updateId->Provider);
    free(updateId->Name);
    free(updateId->Version);
    free(updateId);
}

/**
 * @brief Allocates and sets the UpdateId fields
 * @details Caller should free the allocated ADUC_UpdateId* using ADUC_UpdateId_UninitAndFree()
 * @param provider the provider for the UpdateId
 * @param name the name for the UpdateId
 * @param version the version for the UpdateId
 *
 * @returns An UpdateId on success, NULL on failure
 */
ADUC_UpdateId* ADUC_UpdateId_AllocAndInit(const char* provider, const char* name, const char* version)
{
    _Bool success = false;

    ADUC_UpdateId* updateId = (ADUC_UpdateId*)calloc(1, sizeof(ADUC_UpdateId));

    if (updateId == NULL)
    {
        Log_Error("ADUC_UpdateId_AllocAndInit called with a NULL updateId handle");
        goto done;
    }

    if (provider == NULL || name == NULL || version == NULL)
    {
        Log_Error(
            "Invalid call to ADUC_UpdateId_AllocAndInit with provider %s name %s version %s", provider, name, version);
        goto done;
    }

    if (mallocAndStrcpy_s(&(updateId->Provider), provider) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(updateId->Name), name) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(updateId->Version), version) != 0)
    {
        goto done;
    }

    success = true;

done:

    if (!success)
    {
        ADUC_UpdateId_UninitAndFree(updateId);
        updateId = NULL;
    }

    return updateId;
}
