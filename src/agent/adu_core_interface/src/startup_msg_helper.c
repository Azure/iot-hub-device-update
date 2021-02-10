/**
 * @file startup_msg_helper.h
 * @brief Implements helper functions for building the startup message
 *
 * @copyright Copyright (c) 2021, Microsoft Corporation.
 */

#include "startup_msg_helper.h"

#include <aduc/adu_core_json.h>
#include <aduc/logging.h>
#include <aduc/string_c_utils.h>
#include <azure_c_shared_utility/crt_abstractions.h>

#ifndef ADUC_PLATFORM_SIMULATOR // DO is not used in sim mode
#    include <do_config.h>
#endif

/**
 * @brief Get manufacturer
 * Company name of the device manufacturer.
 * This could be the same as the name of the original equipment manufacturer (OEM).
 * e.g. Contoso
 * @details Caller must free returned value
 *
 * @return char* Value of property allocated with malloc, or NULL on error.
 */
char* DeviceProperties_GetManufacturer()
{
    char* result = NULL;
    char manufacturer[1024];

    if (ReadDelimitedValueFromFile(ADUC_CONF_FILE_PATH, "aduc_manufacturer", manufacturer, ARRAY_SIZE(manufacturer)))
    {
        if (mallocAndStrcpy_s(&result, manufacturer) != 0)
        {
            return NULL;
        }
    }
    else
    {
        // If file doesn't exist, or value wasn't specified, use build default.
        if (mallocAndStrcpy_s(&result, ADUC_DEVICEPROPERTIES_MANUFACTURER) != 0)
        {
            return NULL;
        }
    }

    return result;
}

/**
 * @brief Get device model.
 * Device model name or ID.
 * e.g. Surface Book 2
 * @details Caller must free returned value
 *
 * @return char* Value of property allocated with malloc, or null on error.
 */
char* DeviceProperties_GetModel()
{
    char* result = NULL;
    char model[1024];

    if (ReadDelimitedValueFromFile(ADUC_CONF_FILE_PATH, "aduc_model", model, ARRAY_SIZE(model)))
    {
        if (mallocAndStrcpy_s(&result, model) != 0)
        {
            return NULL;
        }
    }
    else
    {
        // If file doesn't exist, or value wasn't specified, use build default.
        if (mallocAndStrcpy_s(&result, ADUC_DEVICEPROPERTIES_MODEL) != 0)
        {
            return NULL;
        }
    }
    return result;
}

/**
 * @brief Adds the aduc_manufacturer and aduc_model to the @p devicePropsObj
 * @param devicePropsObj the JSON_Object the manufacturer and model will be added to
 * @returns true on successful addition and false on failure
 */
static _Bool DeviceProperties_AddManufacturerAndModel(JSON_Object* devicePropsObj)
{
    bool success = false;

    char* manufacturer = NULL;
    char* model = NULL;

    manufacturer = DeviceProperties_GetManufacturer();

    if (manufacturer == NULL)
    {
        Log_Error("Could not get manufacturer");
        goto done;
    }

    model = DeviceProperties_GetModel();

    if (model == NULL)
    {
        Log_Error("Could not get model");
        goto done;
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
    free(manufacturer);
    free(model);

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
