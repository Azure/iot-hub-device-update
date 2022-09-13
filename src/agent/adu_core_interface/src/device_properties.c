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
 * @brief The ADU contract model id associated with the modelId for agent-orchestrated updates.
 */
#define ADUC_DEVICEPROPERTIES_DEVICEUPDATE_CONTRACT_MODEL_ID "dtmi:azure:iot:deviceUpdateContractModel;2"

/* @brief The adu client builder and version.
 * Consisting of BUILDER; component and ADUC_VERSION
 * e.g. DU;agent/0.3.0-private-preview
 */
#define ADUC_BUILDER_VERSION ADUC_BUILDER_IDENTIFIER ";agent/" ADUC_VERSION

/**
 * @brief Adds the aduc_manufacturer and aduc_model to the @p devicePropsObj
 * @param devicePropsObj the JSON_Object the manufacturer and model will be added to
 * @param agent the ADUC_AgentInfo that contains the agent info
 * @returns true on successful addition and false on failure
 */
_Bool DeviceProperties_AddManufacturerAndModel(JSON_Object* devicePropsObj, const ADUC_AgentInfo* agent)
{
    bool success = false;
    bool configExisted = false;

    char* manufacturer = NULL;
    char* model = NULL;

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

    if (!configExisted)
    {
        // If file doesn't exist, or value wasn't specified, use build default.
        Log_Info("Config file doesn't exist, use build default for manufacturer and model.'");
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
    return success;
}

/**
 * @brief Clears the interfaceId property from the @p devicePropsObj.
 *     This function add or update the value of 'interfaceId' property in @p devicePropsObj to null.
 *  When client reports any property with null value to the IoTHub device (or, module) twin, the property will
 *  be deleted from the twin.
 *
 * @param devicePropsObj the JSON_Object the interfaceId will be set to 'null'.
 * @returns true on successful addition and false on failure
 */
_Bool DeviceProperties_ClearInterfaceId(JSON_Object* devicePropsObj)
{
    bool success = false;

    JSON_Status jsonStatus = json_object_set_null(
        devicePropsObj,
        ADUCITF_FIELDNAME_DEVICEPROPERTIES_INTERFACEID);

    if (jsonStatus != JSONSuccess)
    {
        Log_Error(
            "Could not set JSON field '%s' to null",
            ADUCITF_FIELDNAME_DEVICEPROPERTIES_INTERFACEID);
        goto done;
    }

    success = true;
done:
    return success;
}

/**
 * @brief Adds the contractModelId property into the @p devicePropsObj
 * @param devicePropsObj the JSON_Object the contractModelId will be added to
 * @returns true on successful addition and false on failure
 */
_Bool DeviceProperties_AddContractModelId(JSON_Object* devicePropsObj)
{
    bool success = false;

    JSON_Status jsonStatus = json_object_set_string(
        devicePropsObj,
        ADUCITF_FIELDNAME_DEVICEPROPERTIES_CONTRACT_MODEL_ID,
        ADUC_DEVICEPROPERTIES_DEVICEUPDATE_CONTRACT_MODEL_ID);

    if (jsonStatus != JSONSuccess)
    {
        Log_Error(
            "Could not serialize JSON field: %s value: %s",
            ADUCITF_FIELDNAME_DEVICEPROPERTIES_CONTRACT_MODEL_ID,
            ADUC_DEVICEPROPERTIES_DEVICEUPDATE_CONTRACT_MODEL_ID);
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

/**
 * @brief Adds the customized additional device properties to the @p devicePropsObj
 * @param devicePropsObj the JSON_Object the additional device properties will be added to
 * @param agent the ADUC_AgentInfo that contains the agent info
 * @returns true on successful addition and false on failure
 */
_Bool DeviceProperties_AddAdditionalProperties(JSON_Object* devicePropsObj, const ADUC_AgentInfo* agent)
{
    bool success = false;

    // Additional Device Properties is not a mandatory field. If agent is NULL, skip this function
    if (agent != NULL)
    {
        JSON_Object* additional_properties = agent->additionalDeviceProperties;
        if (additional_properties == NULL)
        {
            // No additional properties is set in the configuration file
            success = true;
            goto done;
        }

        size_t propertiesCount = json_object_get_count(additional_properties);
        for (size_t i = 0; i < propertiesCount; i++)
        {
            const char* name = json_object_get_name(additional_properties, i);
            const char* val = json_value_get_string(json_object_get_value_at(additional_properties, i));

            if ((name == NULL) || (val == NULL))
            {
                Log_Error(
                    "Error retrieving the additional device properties name and/or value at element at index=%zu", i);
                goto done;
            }

            JSON_Status jsonStatus = json_object_set_string(devicePropsObj, name, val);
            if (jsonStatus != JSONSuccess)
            {
                Log_Error("Could not serialize JSON field: %s value: %s", name, val);
                goto done;
            }
        }
    }

    success = true;

done:
    if (!success)
    {
        Log_Error("Failed to get customized additional device properties");
    }
    return success;
}
