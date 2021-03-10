/**
 * @file adu_type.h
 * @brief Defines common data types used throughout ADU project.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 */

#include <aduc/adu_types.h>
#include <stdlib.h>
#include <string.h>

void ADUC_ConnectionInfoDeAlloc(ADUC_ConnectionInfo* info)
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


ADUC_ConnType GetConnTypeFromConnectionString(const char* connectionString)
{
    ADUC_ConnType result = ADUC_ConnType_NotSet;

    if (strstr(connectionString,"DeviceId=") != NULL)
    {
        result = ADUC_ConnType_Device;
    }

    if (result == ADUC_ConnType_Device && strstr(connectionString,"ModuleId=") != NULL)
    {
        result = ADUC_ConnType_Module;
    }

    return result;
}