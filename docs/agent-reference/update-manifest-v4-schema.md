# Update Manifest Version 5

## Schema

Please see the [Update Manifest](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/update-manifest) documentation before reviewing the following changes as part of the Public Preview release.

## Multi-Step Ordered Execution (MSOE) Support

Please see the [Multi-Step Update Manifest](https://learn.microsoft.com/en-us/azure/iot-hub-device-update/device-update-multi-step-updates) documentation.

## Parent Update vs. Child Update

For Public Preview Refresh, we will refer to the top-level Update Manifest as `Parent Update` and refer to an Update Manifest specified in a Reference Step as `Child Update`.

Currently, a `Child Update` must not contain any Reference Steps. This restriction is validate at import time. If violated, the import will fail.

### Inline Step In Parent Update

Inline step(s) specified in `Parent Update` will be applied to the Host Device. Here the ADUC_WorkflowData object that is passed to a Step Handler (aka. Update Content Handler) will not contains a `Selected Components` data. The handler for this type of step should not be a `Component-Aware` handler.

> **Note** | See [Steps Content Handler](../../src/extensions/update_manifest_handlers/steps_handler/README.md) and [Implementing a Component-Aware Content Handler](./how-to-implement-custom-update-handler.md#implementing-a-component-aware-content-handler) for more details.

### Reference Step In Parent Update

Reference step(s) specified in `Parent Update` will be applied to the component on or components connected to the Host Device. A **Reference Step** is a step that contains update identifier of another Update, called `Child Update`. When processing a Reference Step, Steps Handler will download a Detached Update Manifest file specified in the Reference Step data, then validate the file integrity.

Next, the Steps Handler will parse the Child Update Manifest and create ADUC_Workflow object (aka. Child Workflow Data) by combining the data from Child Update Manifest and File URLs information from the Parent Update Manifest. This Child Workflow Data also has a 'level' property set to '1'.

> **Note** | For Update Manifest version v4, the Child Update cannot contain any Reference Steps.

## Detached Update Manifest

To avoid deployment failure due to IoT Hub Twin Data Size Limit, any large Update Manifest will be delivered in a form of JSON data file, called 'Detached Update Manifest'.

If an update with large content is imported into Device Update for IoT Hub, the generated Update Manifest will contain an additional payload file called `Detached Update Manifest` which contains a full data of the Update Manifest.

The `UpdateManifest` property in the Device or Module Twin will contains the Detached Update Manifest file information.

When processing PnP Property Changed Event, Device Update Agent will automatically download the Detached Update Manifest file, and create ADUC_WorkflowData object that contain the full Update Manifest data.
