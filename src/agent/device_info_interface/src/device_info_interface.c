/**
 * @file device_info_interface.c
 * @brief Methods to communicate with "dtmi:azure:DeviceManagement:DeviceInformation;1" interface.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#include "aduc/device_info_interface.h"
#include "aduc/c_utils.h"
#include "aduc/client_handle_helper.h"
#include "aduc/device_info_exports.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h" // atoint64t
#include "pnp_protocol.h"
#include <ctype.h> // isalnum
#include <stdlib.h>

// Name of the DeviceInformation component that this device implements.
static const char g_deviceInfoPnPComponentName[] = "deviceInformation";

/**
 * @brief Handle for DeviceInformation component to communicate to service.
 */
ADUC_ClientHandle g_iotHubClientHandleForDeviceInfoComponent;

//
// DeviceInfoInterfaceData
//

/**
 * @brief Defines the type of a property in the DeviceInformation interface.
 */
typedef enum tagDeviceInfoInterfaceDataType
{
    DIIDT_String, /**< "schema": "string" */
    DIIDT_Long, /**< "schema": "long" */
} DeviceInfoInterfaceDataType;

/**
 * @brief Defines the properties and values of data contained in the DeviceInformation interface.
 */
// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
typedef struct tagDeviceInfoInterfaceData
{
    const DI_DeviceInfoProperty Property; /**< Field enumeration value. */
    const char* PropertyName; /**< Field name of property in interface definition */
    const DeviceInfoInterfaceDataType Type; /**< Field type of property */

    // Values computed at runtime:
    char* Value; /**< Value of property, or NULL if not yet determined. */
    _Bool IsDirty; /**< Specifies if Value has changed since last sent to service. */
} DeviceInfoInterface_Data;

// The names in this struct must match the property names defined in "urn:azureiot:DeviceManagement:DeviceInformation:1".
static DeviceInfoInterface_Data deviceInfoInterface_Data[] = {
    { DIIP_Manufacturer, "manufacturer", DIIDT_String },
    { DIIP_Model, "model", DIIDT_String },
    { DIIP_OsName, "osName", DIIDT_String },
    { DIIP_SoftwareVersion, "swVersion", DIIDT_String },
    { DIIP_ProcessorArchitecture, "processorArchitecture", DIIDT_String },
    { DIIP_ProcessorManufacturer, "processorManufacturer", DIIDT_String },
    { DIIP_TotalMemory, "totalMemory", DIIDT_Long },
    { DIIP_TotalStorage, "totalStorage", DIIDT_Long },
};

/**
 * @brief Free the members in the device info interface struct.
 */
void DeviceInfoInterfaceData_Free()
{
    for (unsigned index = 0; index < ARRAY_SIZE(deviceInfoInterface_Data); ++index)
    {
        DeviceInfoInterface_Data* data = deviceInfoInterface_Data + index;

        free(data->Value);
        data->Value = NULL;

        data->IsDirty = false;
    }
}

/**
 * @brief Ensure that a DeviceInfo property meets certain constraints.
 *
 * For private preview, Manufacturer and Model have the following limitations:
 * 1. 1-64 characters in length.
 * 2. alphanumeric, dot and dash only.
 * However, to be safe, apply these constraints to all properties.
 *
 * @param value String value to apply constraints to.
 */
static void ApplyDeviceInfoPropertyConstraints(char* value)
{
    const unsigned int max_cch = 64;
    char* curr = value;
    while (*curr != '\0')
    {
        if ((curr - value) == max_cch)
        {
            // Force truncation at max_cch characters.
            *curr = '\0';
            break;
        }

        if (!(isalnum(*curr) || *curr == '.' || *curr == '-'))
        {
            // Replace unsupported characters with '-'
            *curr = '-';
        }
        ++curr;
    }
}

/**
 * @brief Refresh Device Info Interface Data object.
 *
 */
static void RefreshDeviceInfoInterfaceData()
{
    for (unsigned index = 0; index < ARRAY_SIZE(deviceInfoInterface_Data); ++index)
    {
        DeviceInfoInterface_Data* data = deviceInfoInterface_Data + index;

        // Call exported upper-level method to get the property value.
        char* value = DI_GetDeviceInformationValue(data->Property);
        if (value == NULL)
        {
            // NULL indicates failure or value not changed, so skip.
            continue;
        }

        ApplyDeviceInfoPropertyConstraints(value);
        free(data->Value);
        data->Value = value;
        data->IsDirty = true;

        Log_Info("Property %s changed to %s", data->PropertyName, data->Value);
    }
}

//
// DeviceInfoInterface methods
//

/**
 * @brief Create a DeviceInfoInterface object.
 *
 * @param componentContext Context object to use for related calls.
 * @param argc Count of arguments in @p argv
 * @param argv Command line parameters.
 * @return _Bool True on success.
 */
_Bool DeviceInfoInterface_Create(void** componentContext, int argc, char** argv)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    *componentContext = NULL;

    return true;
}

/**
 * @brief Called after connected to IoTHub (device client handler is valid).
 *
 * @param componentContext Context object from Create.
 */
void DeviceInfoInterface_Connected(void* componentContext)
{
    UNREFERENCED_PARAMETER(componentContext);

    Log_Info("DeviceInformation component is ready - reporting properties");

    //
    // After DeviceInfoInterface is registered, report current DeviceInfo properties, e.g. software version.
    //

    IOTHUB_CLIENT_RESULT reportResult = DeviceInfoInterface_ReportChangedPropertiesAsync();
    if (reportResult != IOTHUB_CLIENT_OK)
    {
        Log_Warn("DeviceInfoInterface_ReportChangedPropertiesAsync() failed, %u", reportResult);
    }
}

void DeviceInfoInterface_Destroy(void** componentContext)
{
    UNREFERENCED_PARAMETER(componentContext);

    // context isn't used, as we reference the global deviceInfoInterface_Data.
    DeviceInfoInterfaceData_Free();
}

IOTHUB_CLIENT_RESULT ReportChangedProperty(DeviceInfoInterface_Data* data)
{
    ADUC_ClientHandle deviceClientLL = g_iotHubClientHandleForDeviceInfoComponent;
    IOTHUB_CLIENT_RESULT iothubClientResult = IOTHUB_CLIENT_OK;
    STRING_HANDLE jsonToSend = NULL;

    if (!data->IsDirty)
    {
        goto done;
    }

    const char* propertyName = data->PropertyName;
    const char* propertyValue = data->Value;

    Log_Info("Reporting changed property: %s, value: %s", propertyName, propertyValue);

    jsonToSend = PnP_CreateReportedProperty(g_deviceInfoPnPComponentName, propertyName, propertyValue);

    if (jsonToSend == NULL)
    {
        Log_Error(
            "Unable to build reported property response for propertyName=%s, propertyValue=%s",
            propertyName,
            propertyValue);
        iothubClientResult = IOTHUB_CLIENT_ERROR;
        goto done;
    }

    const char* jsonToSendStr = STRING_c_str(jsonToSend);
    size_t jsonToSendStrLen = strlen(jsonToSendStr);

    iothubClientResult = ClientHandle_SendReportedState(
        deviceClientLL, (const unsigned char*)jsonToSendStr, jsonToSendStrLen, NULL, NULL);

    if (iothubClientResult != IOTHUB_CLIENT_OK)
    {
        Log_Error(
            "DeviceInfoInterface: Reporting property %s failed, error: %d, %s",
            propertyName,
            iothubClientResult,
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_RESULT, iothubClientResult));
        goto done;
    }

done:
    if (jsonToSend != NULL)
    {
        STRING_delete(jsonToSend);
    }

    return iothubClientResult;
}

IOTHUB_CLIENT_RESULT DeviceInfoInterface_ReportChangedPropertiesAsync()
{
    IOTHUB_CLIENT_RESULT iothubClientResult = IOTHUB_CLIENT_OK;
    RefreshDeviceInfoInterfaceData();

    STRING_HANDLE jsonToSend = NULL;
    char* serialized_string = NULL;
    JSON_Value* root_value = json_value_init_object();
    JSON_Object* root_object = json_value_get_object(root_value);

    const char pnpReportedPropertyFormat[] = "{\"%s\":%s}";

    json_object_set_string(root_object, "__t", "c");

    for (unsigned index = 0; index < ARRAY_SIZE(deviceInfoInterface_Data); ++index)
    {
        DeviceInfoInterface_Data* data = deviceInfoInterface_Data + index;

        const char* propertyName = data->PropertyName;
        const char* propertyValue = data->Value;

        if (data->Type == DIIDT_String)
        {
            json_object_set_string(root_object, propertyName, propertyValue);
        }
        else
        {
            unsigned long val = 0;
            if (!atoul(propertyValue, &val))
            {
                Log_Error("Cannot convert property value to number. Value: %s", propertyValue);
                iothubClientResult = IOTHUB_CLIENT_ERROR;
                goto done;
            }
            json_object_set_number(root_object, propertyName, val);
        }
    }

    serialized_string = json_serialize_to_string(root_value);

    jsonToSend = STRING_construct_sprintf(pnpReportedPropertyFormat, g_deviceInfoPnPComponentName, serialized_string);

    if (jsonToSend == NULL)
    {
        Log_Error("Unable to build reported property for DeviceInformation component.");
        iothubClientResult = IOTHUB_CLIENT_ERROR;
        goto done;
    }

    const char* jsonToSendStr = STRING_c_str(jsonToSend);
    size_t jsonToSendStrLen = strlen(jsonToSendStr);

    iothubClientResult = ClientHandle_SendReportedState(
        g_iotHubClientHandleForDeviceInfoComponent, (const unsigned char*)jsonToSendStr, jsonToSendStrLen, NULL, NULL);

    if (iothubClientResult != IOTHUB_CLIENT_OK)
    {
        Log_Error(
            "DeviceInfoInterface: Report property failed, error: %d, %s",
            iothubClientResult,
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_RESULT, iothubClientResult));
    }

done:
    if (root_value != NULL)
    {
        json_value_free(root_value);
    }

    if (serialized_string)
    {
        json_free_serialized_string(serialized_string);
    }

    if (jsonToSend != NULL)
    {
        STRING_delete(jsonToSend);
    }

    return iothubClientResult;
}
