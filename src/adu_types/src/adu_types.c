/**
 * @file adu_types.c
 * @brief Implements helper functions related to ConnectionInfo
 * @copyright Copyright (c) Microsoft Corporation.
 */

#include <aduc/adu_types.h>
#include <aduc/logging.h>
#include <stdlib.h>
#include <string.h>

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

    if (strstr(connectionString, "DeviceId=") != NULL)
    {
        if (strstr(connectionString, "ModuleId=") != NULL)
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
        Log_Debug(
            "Connection string passed to GetConnTypeFromConnectionString does not contain a DeviceId or ModuleId value");
    }

    return result;
}