# Welcome to the Device Update Agent Private Preview Refresh (PPR) Release

## What's New In This Version?

- New Device Update [Configurations file](./configurations-file.md)
- [Device Diagnostic Service](./device-diagnostic-service.md) Support
- Support new [Update Manifest Version 4](./update-manifest-v4-schema.md), which enables the following features:
  - [Multi Step Ordered Execution](./update-manifest-v4-schema.md#multi-step-ordered-execution-msoe-support) (MSOE)
  - [Multi Component Updating](./multi-component-updating.md)
  - [Goal State](./goal-state-support.md) Deployment
  - [Detached Update Manifest](./update-manifest-v4-schema.md)
- Agent Extensibility Support
  - [Update Content Handler Extension](./how-to-implement-custom-update-handler.md)
    - Added [APT Update Handler](../../src/content_handlers/apt_handler)
    - Added Script Handler
    - Added [Simulator Handler](../../src/content_handlers/simulator_handler)
    - Added [Steps Handler](../../src/content_handlers/steps_handler/README.md)
    - Added [SWUpdate Handler](../../src/content_handlers/swupdate_handler)

  - [Component Enumerator Extension](../../src/extensions/component-enumerators/examples/contoso-component-enumerator/README.md)
  - [Content Downloader Extension](../../src/extensions/content-downloaders/README.md)
  
## Backward Compatibility
  
Device Update Agent PPR supports both Update Manifest version 2 and version 4. This means, the same Agent can process existing updates those were imported and intended for IoT Device with Device Update Agent Public Preview version installed.  

See [Update Manifest Version 4 Schema](./update-manifest-v4-schema.md) for more details.

## Upgrade From Public Preview Agent to the Public Preview Refresh Agent

See the [Upgrade Guide](./upgrade-guide.md) for step-by-step instructions on how to update existing devices to support the new Public Preview Refresh features.

## Device Update Quickstart

[Getting Started With Device Update](quickstarts/overview-quickstart.md)
