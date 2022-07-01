# Welcome to the Device Update Agent Release

## What's New In Version 1.0 (GA)?

- New Step Handler Extension
  - [SWUpdate Handler V2](../../src/extensions/step_handlers/swupdate_handler_v2/README.md)

- Add Extensibility for the Download Handlers
  - [Microsoft Delta Download Handler Example](../../src/extensions/download_handlers/plugin_examples/microsoft_delta_download_handler/../../../../../README.md)

- Component Name Changes
  - contoso-component-enumerator => contoso_component_enumerator
  - curl-content-downloader => curl_content_downloader
  - deliveryoptimization-content-downloader => deliveryoptimization_content_downloader

## What's New In Version 0.8.0 (Public Preview Refresh)?

- New Device Update [Configurations file](./configurations-file.md)
- [Device Diagnostic Service](./device-diagnostic-service.md) Support
- Support new [Update Manifest Version 4](./update-manifest-v4-schema.md), which enables the following features:
  - [Multi Step Ordered Execution](./update-manifest-v4-schema.md#multi-step-ordered-execution-msoe-support) (MSOE)
  - [Multi Component Updating](./multi-component-updating.md)
  - [Goal State](./goal-state-support.md) Deployment
  - [Detached Update Manifest](./update-manifest-v4-schema.md)
- Agent Extensibility Support
  - [Update Content Handler Extension](./how-to-implement-custom-update-handler.md)
    - Added [APT Update Handler](../../src/extensions/step_handlers/apt_handler)
    - Added Script Handler
    - Added [Simulator Handler](../../src/extensions/step_handlers/simulator_handler)
    - Added [Steps Handler](../../src/extensions/step_handlers/steps_handler/README.md)
    - Added [SWUpdate Handler](../../src/extensions/step_handlers/swupdate_handler)

  - [Component Enumerator Extension](../../src/extensions/component_enumerators/examples/contoso_component_enumerator/README.md)
  - [Content Downloader Extension](../../src/extensions/content_downloaders/README.md)
  
## Backward Compatibility
  
Device Update Agent PPR supports both Update Manifest version 2 and version 4. This means, the same Agent can process existing updates those were imported and intended for IoT Device with Device Update Agent Public Preview version installed.  

See [Update Manifest Version 4 Schema](./update-manifest-v4-schema.md) for more details.

## Upgrade From Public Preview Agent to the Public Preview Refresh Agent

See the [Upgrade Guide](./upgrade-guide.md) for step-by-step instructions on how to update existing devices to support the new Public Preview Refresh features.

## Device Update Quickstart

[Getting Started With Device Update](quickstarts/overview-quickstart.md)
