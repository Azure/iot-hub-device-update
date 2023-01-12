/**
 * @file contoso_component_enumerator.cpp
 * @brief Implementation of an example component enumerator.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/component_enumerator_extension.hpp"
#include "parson.h"
#include <aduc/contract_utils.h>
#include <algorithm>
#include <sstream>
#include <string.h>

/*

    Example component-inventory file:

    {
    "components" : [
        {
            "id" : "0",
            "name" : "host-fw",
            "group" : "firmware",
            "manufacturer" : "contoso",
            "model" : "virtual-adu-device-1",
            "properties" : {
                "path" : "/tmp/virtual-adu-device/fw",
                "updateDataFile" : "firmware.json"
            }
        },
        ...
        ]
    }
    */

//
// *** IMPORTANT NOTE ****
// For demonstration purposes, this is a fixed location for component inventory file.
// And should be override.
//
// To implement the actual enumerator, all components should be enumerated directly
// using system APIs or commands.
//
const char* g_contosoComponentInventoryFilePath = "/usr/local/contoso-devices/components-inventory.json";

static JSON_Value* _GetAllComponentsFromFile(const char* configFilepath)
{
    // Read config file.
    JSON_Value* rootValue = json_parse_file(configFilepath);
    if (rootValue == nullptr)
    {
        printf("Cannot parse component configuration files ('%s').", configFilepath);
        return nullptr;
    }

    JSON_Object* rootObject = json_value_get_object(rootValue);

    // Get components array.
    JSON_Array* components = json_object_get_array(rootObject, "components");
    if (components == nullptr)
    {
        json_value_free(rootValue);
        rootValue = nullptr;
        return nullptr;
    }

    // Populate each components' properties.
    for (size_t i = 0; i < json_array_get_count(components);)
    {
        JSON_Object* component = json_array_get_object(components, i);
        JSON_Object* properties = json_object_get_object(component, "properties");
        if (properties != nullptr)
        {
            // Populate 'properties.status'
            //    'unknown' : 'firmwareDataFile' file is missing
            //    'ok'     : 'firmwareDataFile' file exists
            const char* path = json_object_get_string(properties, "path");
            const char* firmwareDataFile = json_object_get_string(properties, "firmwareDataFile");

            // Note: for demonstration purposes, we will populate 'properties' object with all
            // data (name-value pairs) from specified (mock) firmware data file.
            // And set component's status to 'ok'.
            if (path != nullptr && firmwareDataFile != nullptr)
            {
                std::stringstream propsFile;
                propsFile << path << "/" << firmwareDataFile;

                // Read properties from firmware data file.
                JSON_Value* propsValues = json_parse_file(propsFile.str().c_str());
                if (propsValues == nullptr)
                {
                    // Simulator: cannot communicate with the component.
                    // Set 'status' to 'unknown'.
                    if (JSONFailure == json_object_set_string(properties, "status", "unknown"))
                    {
                        printf("Cannot add 'status (unknown)' property to component #%d\n", static_cast<int>(i));
                    }
                    i++;
                    continue;
                }

                if (JSONFailure == json_object_set_string(properties, "status", "ok"))
                {
                    printf("Cannot add 'status (ok)' property to component #%d\n", static_cast<int>(i));
                }

                JSON_Object* props = json_object(propsValues);
                for (size_t pv = 0; pv < json_object_get_count(props); pv++)
                {
                    const char* key = json_object_get_name(props, pv);
                    if (strcmp(key, "properties") != 0)
                    {
                        JSON_Value* val = json_value_deep_copy(json_object_get_value_at(props, pv));
                        if (val != nullptr)
                        {
                            if (JSONFailure == json_object_set_value(component, key, val))
                            {
                                json_value_free(val);
                                printf(
                                    "Cannot set value '%s' from firmware data file '%s'",
                                    key,
                                    propsFile.str().c_str());
                            }
                        }
                    }
                }

                json_value_free(propsValues);
            }
        }
        i++;
    }

    return rootValue;
}

static bool _json_object_contains_named_value(JSON_Object* jsonObject, const char* name, const char* value)
{
    if (!(jsonObject != nullptr && name != nullptr && *name != 0 && value != nullptr && *value != 0))
    {
        return false;
    }

    return (
        json_object_has_value_of_type(jsonObject, name, JSONString) != 0
        && strcmp(json_object_get_string(jsonObject, name), value) == 0);
}

EXTERN_C_BEGIN

/////////////////////////////////////////////////////////////////////////////
// BEGIN Shared Library Export Functions
//
// These are the function symbols that the device update agent will
// lookup and call.
//

/**
 * @brief Select component(s) that contain property or properties matching specified in @p selectorJson string.
 *
 * Example input json:
 *      - Select all components belong to a 'Motors' group
 *              "{\"group\":\"Motors\"}"
 *
 *      - Select a component with name equals 'left-motor'
 *              "{\"name\":\"left-motor\"}"
 *
 *      - Select components matching specified class (manufature/model)
 *              "{\"manufacturer\":\"Contoso\",\"model\":\"USB-Motor-0001\"}"
 *
 * @param selectorJson A stringified json containing one or more properties use for components selection.
 * @return Returns a serialized json data containing components information.
 * Caller must call FreeString function when done with the returned string.
 */
char* SelectComponents(const char* selectorJson)
{
    char* outputString = nullptr;

    JSON_Value* allComponentsValue = nullptr;
    JSON_Array* componentsArray = nullptr;

    JSON_Value* selectorValue = json_parse_string(selectorJson);
    JSON_Object* selector = json_object(selectorValue);
    if (selector == nullptr)
    {
        goto done;
    }

    // NOTE: For demonstration purposes, we're popoulating components data by reading from
    // the specified 'component inventory' file.
    allComponentsValue = _GetAllComponentsFromFile(g_contosoComponentInventoryFilePath);
    componentsArray = json_object_get_array(json_object(allComponentsValue), "components");
    if (componentsArray == nullptr)
    {
        goto done;
    }

    // Keep only components that contain all properties (name & value) specified in the selector.
    for (int i = json_array_get_count(componentsArray) - 1; i >= 0; i--)
    {
        JSON_Object* component = json_array_get_object(componentsArray, i);
        for (int s = json_object_get_count(selector) - 1; s >= 0; s--)
        {
            bool matched = _json_object_contains_named_value(
                component, json_object_get_name(selector, s), json_string(json_object_get_value_at(selector, s)));
            if (!matched)
            {
                json_array_remove(componentsArray, (size_t)i);
            }
        }
    }

    outputString = json_serialize_to_string_pretty(allComponentsValue);

done:
    json_value_free(selectorValue);
    json_value_free(allComponentsValue);

    return outputString;
}

/**
 * @brief Returns all components information in JSON format.
 * @param includeProperties Indicates whether to include optional component's properties in the output string.
 * @return Returns a serialized json data contains components information. Caller must call FreeComponentsDataString function
 * when done with the returned string.
 */
char* GetAllComponents()
{
    JSON_Value* val = _GetAllComponentsFromFile(g_contosoComponentInventoryFilePath);
    char* returnString = json_serialize_to_string_pretty(val);
    json_value_free(val);
    return returnString;
}

/**
 * @brief Frees the components data string allocated by GetAllComponents.
 *
 * @param string The string previously returned by GetAllComponents.
 */
void FreeComponentsDataString(char* string)
{
    json_free_serialized_string(string);
}

/**
 * @brief Gets the extension contract info.
 *
 * @param[out] contractInfo The extension contract info.
 * @return ADUC_Result The result.
 */
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo)
{
    contractInfo->majorVer = ADUC_V1_CONTRACT_MAJOR_VER;
    contractInfo->minorVer = ADUC_V1_CONTRACT_MINOR_VER;
    return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
}

//
// END Shared Library Export Functions
/////////////////////////////////////////////////////////////////////////////

EXTERN_C_END
