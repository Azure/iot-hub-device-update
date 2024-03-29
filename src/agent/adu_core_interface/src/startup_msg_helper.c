/**
 * @file startup_msg_helper.h
 * @brief Implements helper functions for building the startup message
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "startup_msg_helper.h"
#include "device_properties.h"

#include <aduc/logging.h>
#include <aduc/string_c_utils.h>
#include <aduc/types/update_content.h> // ADUCITF_FIELDNAME_DEVICEPROPERTIES, etc.
#include <azure_c_shared_utility/crt_abstractions.h>

/**
 * @brief The default compatibility properties sent to the cloud via DeviceProperties compatPropertyNames.
 */
#define DEFAULT_COMPAT_PROPERTY_NAMES_VALUE "manufacturer,model"

/**
 * @brief Adds the deviceProperties to the @p startupObj
 * @param startupObj the JSON Object which will have the device properties added to it
 * @param agent the ADUC_AgentInfo that contains the agent info
 * @returns true on successful addition, false on failure
 */
bool StartupMsg_AddDeviceProperties(JSON_Object* startupObj, const ADUC_AgentInfo* agent)
{
    if (startupObj == NULL)
    {
        return false;
    }

    bool success = false;

    JSON_Value* devicePropsValue = json_value_init_object();

    JSON_Object* devicePropsObj = json_value_get_object(devicePropsValue);

    if (devicePropsObj == NULL)
    {
        goto done;
    }

    if (!DeviceProperties_AddManufacturerAndModel(devicePropsObj, agent))
    {
        goto done;
    }

    if (!DeviceProperties_AddAdditionalProperties(devicePropsObj, agent))
    {
        goto done;
    }

    if (!DeviceProperties_ClearInterfaceId(devicePropsObj))
    {
        goto done;
    }

    if (!DeviceProperties_AddContractModelId(devicePropsObj))
    {
        goto done;
    }

#ifdef ENABLE_ADU_TELEMETRY_REPORTING
    if (!DeviceProperties_AddVersions(devicePropsObj))
    {
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
        Log_Error("Adding deviceProperties properties failed.");
        json_value_free(devicePropsValue);
    }
    return success;
}

/**
 * @brief Adds the compatPropertyNames to the @p startupObj
 * @param startupObj the JSON Object which will have the compatPropertyNames from config added to it
 * @remark This function requires that the ADUC_ConfigInfo singleton has been initialized.
 * @returns true on successful addition, false on failure
 */
bool StartupMsg_AddCompatPropertyNames(JSON_Object* startupObj)
{
    if (startupObj == NULL)
    {
        return false;
    }

    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    if (config == NULL)
    {
        Log_Error("ADUC_ConfigInfo singleton hasn't been initialized");
        return false;
    }

    bool success = false;

    JSON_Status jsonStatus = json_object_set_string(
        startupObj,
        ADUCITF_FIELDNAME_COMPAT_PROPERTY_NAMES,
        IsNullOrEmpty(config->compatPropertyNames) ? DEFAULT_COMPAT_PROPERTY_NAMES_VALUE
                                                   : config->compatPropertyNames);

    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_COMPAT_PROPERTY_NAMES);
        goto done;
    }

    success = true;

done:

    ADUC_ConfigInfo_ReleaseInstance(config);
    return success;
}
