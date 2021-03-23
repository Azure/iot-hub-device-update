# Multi Component Update Content Handler

A Multi Component Update Content Handler is an [`Update Content Handler`](../agent-core/update-content-handler.md) extension that can process the `microsoft/mcu:1` update type.  

## Quick Jump

- [Multi Component Update Content Handler](#multi-component-update-content-handler)
  - [Quick Jump](#quick-jump)
    - [MCU Update Workflow](#mcu-update-workflow)
    - [MCU Component Update Workflow](#mcu-component-update-workflow)
    - [How MCU Update Content Handler Locates Files](#how-mcu-update-content-handler-locates-files)
      - [Previous](#previous)
      - [Next](#next)

### MCU Update Workflow

![MCU Workflow Diagram](./diagrams/mcu-sequence-diagram-main.svg)

### MCU Component Update Workflow

![MCU Component Update Workflow Diagram](./diagrams/mcu-sequence-diagram-process-component-updates.svg)

### How MCU Update Content Handler Locates Files

The MCU Content Handler locates the file using `fileUri` property. ( See [File Information Property](./mcu-manifest.md#file-information-property) for detail )

- For script files, if the specified file does not exist, MCU Update Handler will try to download (if needed) and unpack the scripts bundle into the `sandbox` directory. Then, MCU Update Handler will search for the file again. If failed, MCU Update Handler will report `FILE_NOT_FOUND` error.

- For update files, e.g., `device firmware file`, `bootfs image file`, `rootfs image file`, or other `component's firmware image`, MCU Update Handler will search for the specified file in `ADU Update Manifest` `fileUrls` list. If found, MCU Update Handler will requests ADU Agent to download the file into the `sandbox` folder (via Delivery Optimization Agent)

#### Previous

- [MCU Overview](./overview.md)
  
#### Next

- [MCU Manifest File](./mcu-manifest.md)
