/**
 * @file device_properties.c
 * @brief The implementation of device properties functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "device_properties.h"
#include <aduc/logging.h>
#include <aduc/types/update_content.h>

#ifndef ADUC_PLATFORM_SIMULATOR // DO is not used in sim mode
#    include <do_config.h>
#endif

#include <stdlib.h>

/*
 * @brief The ADU interface id associated with the modelId for agent-orchestrated updates.
 */
#define ADUC_DEVICEPROPERTIES_INTERFACEID "dtmi:azure:iot:deviceUpdate;1"

/* @brief The adu client builder and version.
 * Consisting of BUILDER; component and ADUC_VERSION
 * e.g. DU;agent/0.3.0-private-preview
 */
#define ADUC_BUILDER_VERSION ADUC_BUILDER_IDENTIFIER ";agent/" ADUC_VERSION

/**
 * @brief Adds the aduc_manufacturer and aduc_model to the @p devicePropsObj
 * @param devicePropsObj the JSON_Object the manufacturer and model will be added to
 * @returns true on successful addition and false on failure
 */
_Bool DeviceProperties_AddManufacturerAndModel(JSON_Object* devicePropsObj)
{
    bool success = false;
    bool configExisted = false;

    char* manufacturer = NULL;
    char* model = NULL;

    ADUC_ConfigInfo config = {};

    if (ADUC_ConfigInfo_Init(&config, ADUC_CONF_FILE_PATH))
    {
        const ADUC_AgentInfo* agent = ADUC_ConfigInfo_GetAgent(&config, 0);
        if (agent != NULL && agent->manufacturer != NULL && agent->model != NULL)
        {
            configExisted = true;
            if (mallocAndStrcpy_s(&manufacturer, agent->manufacturer) != 0)
            {
                goto done;
            }
            if (mallocAndStrcpy_s(&model, agent->model) != 0)
            {
                goto done;
            }
        }
    }

    if (!configExisted)
    {
        // If file doesn't exist, or value wasn't specified, use build default.
        if (mallocAndStrcpy_s(&manufacturer, ADUC_DEVICEPROPERTIES_MANUFACTURER) != 0)
        {
            goto done;
        }
        if (mallocAndStrcpy_s(&model, ADUC_DEVICEPROPERTIES_MODEL) != 0)
        {
            goto done;
        }
    }

    JSON_Status jsonStatus =
        json_object_set_string(devicePropsObj, ADUCITF_FIELDNAME_DEVICEPROPERTIES_MANUFACTURER, manufacturer);

    if (jsonStatus != JSONSuccess)
    {
        Log_Error(
            "Could not serialize JSON field: %s value: %s",
            ADUCITF_FIELDNAME_DEVICEPROPERTIES_MANUFACTURER,
            manufacturer);
        goto done;
    }

    jsonStatus = json_object_set_string(devicePropsObj, ADUCITF_FIELDNAME_DEVICEPROPERTIES_MODEL, model);

    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not serialize JSON field: %s value: %s", ADUCITF_FIELDNAME_DEVICEPROPERTIES_MODEL, model);
        goto done;
    }

    success = true;

done:
    if (!success)
    {
        Log_Error("Failed to get manufacturer and model device properties");
    }
    ADUC_ConfigInfo_UnInit(&config);
    return success;
}

/**
 * @brief Adds the interfaceId property into the @p devicePropsObj
 * @param devicePropsObj the JSON_Object the interfaceId will be added to
 * @returns true on successful addition and false on failure
 */
_Bool DeviceProperties_AddInterfaceId(JSON_Object* devicePropsObj)
{
    bool success = false;

    JSON_Status jsonStatus = json_object_set_string(devicePropsObj, ADUCITF_FIELDNAME_DEVICEPROPERTIES_INTERFACEID, ADUC_DEVICEPROPERTIES_INTERFACEID);

    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not serialize JSON field: %s value: %s", ADUCITF_FIELDNAME_DEVICEPROPERTIES_INTERFACEID, ADUC_DEVICEPROPERTIES_INTERFACEID);
        goto done;
    }

    success = true;

done:

    return success;
}

/**
 * @brief Adds the version properties into the @p devicePropsObj
 * @param devicePropsObj the JSON_Object the versions will be added to
 * @returns true on successful addition and false on failure
 */
_Bool DeviceProperties_AddVersions(JSON_Object* devicePropsObj)
{
    _Bool success = false;
    char* do_version = NULL;

    JSON_Status jsonStatus =
        json_object_set_string(devicePropsObj, ADUCITF_FIELDNAME_DEVICEPROPERTIES_ADUC_VERSION, ADUC_BUILDER_VERSION);

    if (jsonStatus != JSONSuccess)
    {
        Log_Error(
            "Could not serialize JSON field: %s value: %s",
            ADUCITF_FIELDNAME_DEVICEPROPERTIES_ADUC_VERSION,
            ADUC_BUILDER_VERSION);
        goto done;
    }

#ifndef ADUC_PLATFORM_SIMULATOR
    do_version = deliveryoptimization_get_components_version();

    if (do_version == NULL)
    {
        Log_Warn("Could not get do_version");
        goto done;
    }

    jsonStatus = json_object_set_string(devicePropsObj, ADUCITF_FIELDNAME_DEVICEPROPERTIES_DO_VERSION, do_version);

    if (jsonStatus != JSONSuccess)
    {
        Log_Warn(
            "Could not serialize JSON field: %s value: %s", ADUCITF_FIELDNAME_DEVICEPROPERTIES_DO_VERSION, do_version);
        goto done;
    }
#endif

    success = true;

done:
    free(do_version);
    return success;
}
