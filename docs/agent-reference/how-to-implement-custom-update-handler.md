# How To Implement A Custom Content Handler Extension

## What's A Content Handler Extension

Device Update agent supports an Update Content Handler (a.k.a, Step Handler) extension, which enables the Agent to support multiple Update Types at the same time.

Device builders can define a custom Update Type, implement the associated custom Update Content Handler, and register it, if needed for their additional device updates and deployment scenarios.

## Requirements

1. An extension must be a Linux shared-library that implements and exports following C function:

```c

/**
 * @brief Instantiates an Update Content Handler.
 * @return A pointer to an instantiated Update Content Handler object.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);

```

This function must return a C++ class object that derived from **ContentHandler** abstract class.

See **ContentHandler** class definition in  [content_handler.hpp](../../src/extensions/inc/aduc/content_handler.hpp) for details.

2. The custom content handler must implement all virtual functions. See the details of each functions below:

|Function| Purpose| Return Values|
|----------|----------|----------|
| Download | Handles a 'download' task invoked by DU Agent workflow.<br/><br/>The downloaded file must be stored in a work folder specified in [**ADUC_WorkflowData**](../../src/adu_types/inc/aduc/types/workflow.h) | For success cases, the **ResultCode** field of the **ADUC_Result** struct can be one of the following values:<br/><br/>    ADUC_Result_Download_Success<br/>ADUC_Result_Download_InProgress<br/>ADUC_Result_Download_Skipped_FileExists<br/>ADUC_Result_Download_Skipped_UpdateAlreadyInstalled<br/>ADUC_Result_Download_Skipped_NoMatchingComponents<br/><br/>See [adu_core.h](../../src/adu_types/inc/aduc/types/adu_core.h) for more details |
| Install | Handles an 'install' task invoked by DU Agent workflow.<br/><br/>The install task usually includes a process where the handler invokes external tool or command to install the downloaded update payload file(s) to the desired target.<br/>For example, for 'microsoft/apt' update, the handler could invoke following command:<br/><br/> <b>`apt-get install <options> <list of packages to install>`</b> | For success cases, the **ResultCode** field of the **ADUC_Result** struct can be one of the following values:<br/><br/>    ADUC_Result_Install_Success<br/>ADUC_Result_Install_InProgress<br/>ADUC_Result_Install_Skipped_UpdateAlreadyInstalled<br/>ADUC_Result_Install_Skipped_NoMatchingComponents <br/>ADUC_Result_Install_RequiredImmediateReboot <br/>ADUC_Result_Install_RequiredReboot<br/>ADUC_Result_Install_RequiredImmediateAgentRestart<br/>ADUC_Result_Install_RequiredAgentRestart<br/><br/>See [adu_core.h](../../src/adu_types/inc/aduc/types/adu_core.h) for more details |
| Apply | Handles an 'apply' task invoked by DU Agent workflow.<br/><br/>The apply task usually includes one or more additional steps after an 'install' task has completed. Such as, validating installed items, restart system service, update configuration files, persist some meta data related to the update process.<br/><br/> The DU Agent workflow will consider the update complete successfully only when an apply task returns ADUC_Result_Apply_Success | For success cases, the **ResultCode** field of the **ADUC_Result** struct can be one of the following values:<br/><br/>    ADUC_Result_Apply_Success <br/>ADUC_Result_Apply_InProgress<br/>ADUC_Result_Apply_RequiredImmediateReboot<br/>ADUC_Result_Apply_RequiredReboot <br/>ADUC_Result_Apply_RequiredImmediateAgentRestart <br/>ADUC_Result_Apply_RequiredAgentRestart<br/><br/>See [adu_core.h](../../src/adu_types/inc/aduc/types/adu_core.h) for more details |
| Cancel | Handles a 'cancel' task invoked by DU Agent workflow.<br/><br/>The cancel task usually initiated by the Device Update Service. When an Agent received a cancel request, the Agent workflow will relay this request to an active Upate Content Handler that's currently processing the deployment. The handler should try to gracefully cancelling current task, and return appropriate result. | For success cases, the **ResultCode** field of the **ADUC_Result** struct can be one of the following values:<br/><br/>    ADUC_Result_Cancel_Success <br/>ADUC_Result_Cancel_UnableToCancel<br/<br/>See [adu_core.h](../../src/adu_types/inc/aduc/types/adu_core.h) for more details |
| IsInstalled | In some situation, an Agent workflow can invoke 'IsInstalled' function, to determine whether the current update (as specified in [**ADUC_WorkflowData**](../../src/adu_types/inc/aduc/types/workflow.h) object) is installed on the target device, or component. The Update Content Handler is responsible for evaluate the target state and return result accordingly. | The **ResultCode** field of the **ADUC_Result** struct can be one of the following values:<br/><br/>    ADUC_Result_IsInstalled_Installed <br/>ADUC_Result_IsInstalled_NotInstalled<br/<br/>See [adu_core.h](../../src/adu_types/inc/aduc/types/adu_core.h) for more details |

## Consuming ADUC_WorkflowData

As noted above, DU Agent Core passes an [**ADUC_WorkflowData**](../../src/adu_types/inc/aduc/types/workflow.h) object when invoking a function provided by the handler. A '**WorkflowHandle**' field of the  [**ADUC_WorkflowData**](../../src/adu_types/inc/aduc/types/workflow.h) object contains all data needed to perform each task.

A group of helper functions declared in workflow_utils.h can be used to query workflow data from the WorkflowHandle.

Below, you can find an example of useful helper functions used in most handlers provided in this project.

| Function | Purpose |
|:---|:---|
|workflow_get_update_id|Get the id of an update currently being processed
|workflow_get_update_type| Get the type of an update being processed |
|workflow_get_update_files_count|Get total update payload files count|
|workflow_get_update_file| Get a file entity information.<br/>See [ADUC_FileEntity](../../src/adu_types/inc/aduc/types/update_content.h) for more info.
workflow_get_installed_criteria|Get the 'installed' criteria string|

> See [workflow_utils.h](../../src/utils/workflow_utils/inc/aduc/workflow_utils.h) for full list of the helper functions.

## Implementing A 'Component-Aware' Content Handler

Usually, a content handler is designed to install an update content on a Host Device. An example of this is the [APT Update Content Handler](../../src/extensions/step_handlers/apt_handler/README.md) provided in this project, which installs one or more Debian packages on the host device.

In some case, a Device Builder may want to install an update content on one or more component(s) that connected to the Host Device instead.

In this case, if the Update has been authored and imported correctly, the DU Agent workflow will include a 'Selected Components' data in the ADUC_WorkflowHandle object that is passed to the handler's function.

When implementing a handler that handles an update intended for connected-components, the following helper functions can be used to get selected components properties:

|Function| Purpose|
|---|---|
|workflow_peek_selected_components|Get a serialized JSON string containing a collection of components that the update should be installed.|

The component data is provided by Component Enumerator Extension, which usually implemented by Device Builder and registered on the Host Device.

> See [Contoso Virtual Vacuum Component Enumerator](../../src/extensions/component_enumerators/examples/contoso_component_enumerator/README.md) example for more details

## How To Build A Content Handler Extension

To get started, take a look at existing Content Handler Extensions in [src/content_handlers](../../src/content_handlers) folder for reference.

## How To Register A Content Handler Extension

To register a content handler, run following command on the device:

```sh
sudo /usr/bin/AducIotAgent --extension-type updateContentHandler --register-extension <full path to the handler file> --extension-id <update type name>

# For example
# sudo /usr/bin/AducIotAgent --extension-type updateContentHandler --register-extension /var/lib/adu/extensions/sources/libmicrosoft_apt_1.so --extension-id 'microsoft/apt:1'
```

### Return Value

AducIotAgent will return 0 if it succeeded.
