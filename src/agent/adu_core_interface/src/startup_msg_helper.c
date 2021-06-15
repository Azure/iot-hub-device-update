/**
 * @file startup_msg_helper.h
 * @brief Implements helper functions for building the startup message
 *
 * @copyright Copyright (c) 2021, Microsoft Corporation.
 */

#include "startup_msg_helper.h"

#include <aduc/adu_core_json.h>
#include <aduc/config_utils.h>
#include <aduc/logging.h>
#include <aduc/string_c_utils.h>
#include <azure_c_shared_utility/crt_abstractions.h>

#ifndef ADUC_PLATFORM_SIMULATOR // DO is not used in sim mode
#    include <do_config.h>
#endif

/**
 * @brief Adds the aduc_manufacturer and aduc_model to the @p devicePropsObj
 * @param devicePropsObj the JSON_Object the manufacturer and model will be added to
 * @returns true on successful addition and false on failure
 */
static _Bool DeviceProperties_AddManufacturerAndModel(JSON_Object* devicePropsObj)
{
    bool success = false;
    bool configExisted = false;

    char* manufacturer = NULL;
    char* model = NULL;

    ADUC_ConfigInfo config = {};

    if (ADUC_ConfigInfo_Init(&config, ADUC_CONF_FILE_PATH))
    {
        //TODO(Nox): [MCU work in progress] Currently only supporting one agent (host device), so only reading the first agent
        //in the configuration file. Later it will fork different processes to process multiple agents.
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
 * @brief Get adu client builder and version.
 * Consisting of BUILDER; component and ADUC_VERSION
 * e.g. DU;agent/0.3.0-private-preview
 *
 * @return const char* Value of property
 */
const char* DeviceProperties_GetAducBuilderVersion()
{
    return ADUC_BUILDER_IDENTIFIER ";agent/" ADUC_VERSION;
}

static _Bool DeviceProperties_AddVersions(JSON_Object* devicePropsObj)
{
    _Bool success = false;
    char* do_version = NULL;
    const char* aduc_version = DeviceProperties_GetAducBuilderVersion();

    JSON_Status jsonStatus =
        json_object_set_string(devicePropsObj, ADUCITF_FIELDNAME_DEVICEPROPERTIES_ADUC_VERSION, aduc_version);

    if (jsonStatus != JSONSuccess)
    {
        Log_Error(
            "Could not serialize JSON field: %s value: %s",
            ADUCITF_FIELDNAME_DEVICEPROPERTIES_ADUC_VERSION,
            aduc_version);
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

/**
 * @brief Adds the deviceProperties to the @p startupObj 
 * @param startupObj the JSON Object which will have the device properties added to it
 * @returns true on successful addition, false on failure
 */
_Bool StartupMsg_AddDeviceProperties(JSON_Object* startupObj)
{
    if (startupObj == NULL)
    {
        return false;
    }

    _Bool success = false;

    JSON_Value* devicePropsValue = json_value_init_object();

    JSON_Object* devicePropsObj = json_value_get_object(devicePropsValue);

    if (devicePropsObj == NULL)
    {
        goto done;
    }

    if (!DeviceProperties_AddManufacturerAndModel(devicePropsObj))
    {
        Log_Error("Could not add manufacturer and model to device props");
        goto done;
    }

#ifdef ENABLE_ADU_TELEMETRY_REPORTING
    if (!DeviceProperties_AddVersions(devicePropsObj))
    {
        Log_Error("Could not add do and adu version to device props");
    }
#endif

    JSON_Status jsonStatus = json_object_set_value(startupObj, ADUCITF_FIELDNAME_DEVICEPROPERTIES, devicePropsValue);

    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not serialize JSON field: %s", ADUCITF_FIELDNAME_DEVICEPROPERTIES);
        goto done;
    }

    success = true;
done:

    if (!success)
    {
        json_value_free(devicePropsValue);
    }

    return success;
}
