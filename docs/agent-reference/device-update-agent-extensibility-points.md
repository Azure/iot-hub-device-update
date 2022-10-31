
# Device Update Agent Extensibility Points

The core DeviceUpdate agent is essentially a daemon executable that:

- Connects to IotHub,
- Receives property updates when the twin changes,
- Evaluates "update actions" according to the [device update protocol](./goal-state-support.md) in the "desired" section of the twin
- Delegates processing of the update metadata to a set of extension plugins,
- and Reports state changes and result codes by patching the twin's "reported" section according to the [device update protocol](./goal-state-support.md)

The remainder of this document provides a brief overview of the extensibility points, info on 1st party extensions that implement them, and links to more information, including how to implement custom ones.

## Contract Versions

Every Extension Type shared library plugin includes a "GetContractInfo" function export symbol that explicitly opts into a particular version of the Agent<->ExtensionType extension contract.  To learn more, see [extension contract versions](./extension-contract-versions.md).

## Registration of Extension Shared Libraries

See the [Registering Device Update Extensions doc](./registering-device-update-extensions.md) for more information about registering shared libraries as extensions for each extensibility point.

## Extension Types

There are currently 5 extension types:

1. Content Handler, also known as Step Handler (Note: singular Step, not plural Steps)
    - This is the "installer technology" for the update content type, but it actually handles all aspects of the update (Is the update installed? How to download it in addition to installing/applying the update).
2. Update Manifest Handler,
    - Deals with how to handle one or more steps in the multi-step update.
    - It is also a Content Handler.
3. Content Downloader,
    - The wrapper for the "download technology" that will actually download update payload files and detached update manifests.
4. Download Handler,
    - A pre-download hook that allows skipping of the update payload if the download handler can produce it by some other means (such as Delta Diffing technology, or "Delta Update").
5. and Component Enumerator
    - Allows getting and querying of component hardware modules for proxy updates.

## Content Handler extension type

The agent defers the business logic for `IsInstalled`, `Download`, `Install`, `Apply`, `Backup`, `Rollback`, and `Cancel` actions to the content handler registered for the update type.

### List of 1st-Party Registered Content Handlers

These are the content handlers that come pre-registered when installing the Debian package and can be found under [src/extensions/step_handlers/](../../src/extensions/step_handlers/):

- apt handler ( [README.md](../../src/extensions/step_handlers/apt_handler/README.md) )
  - limited example that can install/remove one or more APT package updates specified in an apt-manifest.json update payload,
- script handler ( [README.md](../../src/extensions/step_handlers/script_handler/README.md) )
  - run a script that is included as an update payload,
- swupdate (version 1) handler ( [README.md](../../src/extensions/step_handlers/swupdate_handler/README.md) )
  - limited example that runs the adu-swupdate.sh script included with the Debian package. It only works on ext4 file system images that conform to ADU Yocto built images.
- swupdate (version 2) handler ( [README.md](../../src/extensions/step_handlers/swupdate_handler_v2/README.md) )
  - runs a script (specified as "scriptFileName" in the "handlerProperites") that's included as an update payload and that invokes swupdate command-line.
  - Agent will also pass it the path to the downloaded .swu file included as "swuFileName" in the "handlerProperties"

### Implementing a custom content handler

See [How to implement custom update handler](./how-to-implement-custom-update-handler.md) and examples under [src/extensions/step_handlers](../../src/extensions/step_handlers/)

## Update Manifest Handler extension type

The update manifest handler handles the processing of steps in the instructions of a v4 or later update manifest. See `steps` in `instructions` of a [v4+ update manifest](./update-manifest-v4-schema.md) here.

### Default Registered Update Manifest Handler

The default Update Manifest Handler registered in deviceupdate-agent Debian package is the [Steps Handler](../../src/extensions/update_manifest_handlers/steps_handler/README.md) (Note: Plural Steps). The implementation is in [steps_handler.cpp](../../src/extensions/update_manifest_handlers/steps_handler/src/steps_handler.cpp).

[Steps Handler](../../src/extensions/update_manifest_handlers/steps_handler/README.md) is a content handler that applies each action aggregated across every step:

- IsInstalled() is true if every step has been installed (and recursively if there are children component updates),
- Download() downloads the update payloads for every step
- Install() will begin after Download() phase is done and will install every step in the order they appear in the update manifest,
- and after Install phase, Apply() will similarly be done for each step in order.

### Custom steps handling example

If, for example, not every update payload should be downloaded upfront, then a custom UpdateManifest Handler would need to be authored and registered to do the content handler actions for the first step, and then for the next step, and so on. Each step could decide to skip it's actions under certain conditions to avoid unnecessary downloads/processing.

### Implementing a custom Update Manifest Handler

To implement a custom Update Manifest Handler, which is a Content Handler, export the function symbols with signatures defined in [extension_content_handler_export_symbols.h](../../src/extensions/inc/aduc/exports/extension_content_handler_export_symbols.h).

For detailed information, see:

- [update-manifest-v4-schema](./update-manifest-v4-schema.md)
- [How to implement custom update handler](./how-to-implement-custom-update-handler.md).
- [Steps_Handler README.md](../../src/extensions/update_manifest_handlers/steps_handler/README.md)

## Content Downloader extension type

The content downloader extensibility point allows a single shared library to be registered for downloading update payloads and v4+ detached update manifests.

### Default Content Downloader

The debian package registers libmicrosoft_deliveryoptimization_content_downloader.so, which is the shared library specified in [src/extensions/content_downloaders/deliveryoptimization_downloader/CMakeLists.txt](../../src/extensions/content_downloaders/deliveryoptimization_downloader/CMakeLists.txt)

See [DeliveryOptimization github](https://github.com/microsoft/do-client) for more details.

### Implementing Custom Content Downloader

The shared library must export the symbols with function symbols documented in [extension_content_downloader_export_symbols.h](../../src/extensions/inc/aduc/exports/extension_content_downloader_export_symbols.h)

Examples include [deliveryoptimization-content-downloader](../../src/extensions/content_downloaders/deliveryoptimization_downloader/deliveryoptimization_content_downloader.EXPORTS.cpp) and [curl-content-downloader](../../src/extensions/content_downloaders/curl_downloader/curl_content_downloader.EXPORTS.cpp).

## Download Handler extension type

The DownloadHandler extensibility point allows registering a shared library to be called by the core agent when a payload file in a [v5 update manifest](./update-manifest-v5-schema.md) has a `downloadHandlerId` that matches the registered id.  The main idea is that the download handler is called before downloading and if it can produce the update payload file, then the agent can skip the download; otherwise, it falls back to downloading the full update payload file.

See the [download handler README.md](../../src/extensions/download_handlers/README.md) for more information.

### 1st Party DownloadHandler included with Debian package

The DeviceUpdate agent installed via the Debian package will include the [Microsoft Delta Download Handler](../../src/extensions/download_handlers/plugin_examples/microsoft_delta_download_handler/handler/plugin/src/microsoft_delta_download_handler_plugin.EXPORTS.c) plugin.  This download handler implements `Delta Updates` using the [Diff API](https://github.com/Azure/iot-hub-device-update-diff#diff-api) to apply the diff to recreate the update payload that was generated by [Diff Generation](https://github.com/Azure/iot-hub-device-update-diff#diff-generation) from a source swupdate .swu file and the target .swu update payload.

### Implementing Download Handler Extension

A download handler must export symbols with the signatures defined in [extension_download_handler_export_symbols.h](../../src/extensions/inc/aduc/exports/extension_download_handler_export_symbols.h)

Example implementation is [here](../../src/extensions/download_handlers/plugin_examples/microsoft_delta_download_handler/handler/plugin/src/microsoft_delta_download_handler_plugin.EXPORTS.c)

## Component Enumerator

Component Enumerator extension is used for Proxy updates.

Read more about it in [multi-component updating](./multi-component-updating.md).

### Implementing a custom component enumerator extension

A component enumerator must export symbols with signatures defined in [extension_component_enumerator_export_symbols.h](../../src/extensions/inc/aduc/exports/extension_component_enumerator_export_symbols.h)

Example component enumerator can be found in
 [example contoso README.md](../../src/extensions/component_enumerators/examples/contoso_component_enumerator/README.md)
 and [contoso component enumerator demo README.md](../../src/extensions/component_enumerators/examples/contoso_component_enumerator/demo/README.md)
