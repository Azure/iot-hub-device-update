# Update Manifest Version 5

## Schema

Please see the [Update Manifest](https://learn.microsoft.com/en-us/azure/iot-hub-device-update/update-manifest) documentation before reviewing the following changes.

### Official JSON Schemas

The Azure Device Update service publishes JSON Schemas for the **import manifest** (the document you author and submit to Device Update for IoT Hub) on [SchemaStore](https://www.schemastore.org/json/):

| Version | JSON Schema URL | Microsoft Learn reference |
| ------- | --------------- | ------------------------- |
| v4 (`"manifestVersion": "4.0"`) | <https://json.schemastore.org/azure-deviceupdate-import-manifest-4.0.json> | [Import schema](https://learn.microsoft.com/en-us/azure/iot-hub-device-update/import-schema) |
| v5 (`"manifestVersion": "5.0"`) | <https://json.schemastore.org/azure-deviceupdate-import-manifest-5.0.json> | [Import schema](https://learn.microsoft.com/en-us/azure/iot-hub-device-update/import-schema) |

Reference one of these from the `$schema` property of your import manifest to get editor validation and IntelliSense:

```json
{
  "$schema": "https://json.schemastore.org/azure-deviceupdate-import-manifest-5.0.json",
  "manifestVersion": "5.0",
  ...
}
```

> **Note** | The schemas above describe the **import manifest** that you submit to the Device Update service. The service then synthesizes the **update manifest** that is delivered to devices via the IoT Hub twin. The two formats share the same core shape, but the update manifest also carries service-generated fields (e.g. signed JWS, file URLs).

### What changed between v4 and v5

The structural definitions for `updateId`, `compatibility`, `instructions`, `inlineStep`, and `referenceStep` are unchanged between v4 and v5. The differences are confined to the per-file `files` entries and enable the **Download Handler** extensibility point:

| Capability | v4 | v5 |
| ---------- | -- | -- |
| `manifestVersion` | `"4.0"` | `"5.0"` |
| `files[].downloadHandler` (object with `id`, e.g. `"microsoft/delta:1"`) | ❌ Not supported | ✅ Optional — opts a payload file into a registered Download Handler so the agent can produce the payload from another source instead of downloading it in full |
| `files[].relatedFiles` (**JSON object/map** keyed by related-file id; each value has its own `fileName`, `sizeInBytes`, `hashes`, and **required** freeform `properties`) | ❌ Not supported | ✅ Optional — declares additional payloads consumed by the download handler (for example, the delta file plus `microsoft.sourceFileHashAlgorithm` / `microsoft.sourceFileHash` properties used by the [Microsoft Delta Download Handler](./delta-download-handler.md)) |

In short, **v5 = v4 plus the ability to attach a Download Handler and `relatedFiles` to any payload file**. Existing v4 manifests remain valid as v5 manifests after bumping `manifestVersion` to `"5.0"`.

See [Download Handler Extensibility Point](../../src/extensions/download_handlers/README.md), [Device Update Agent Extensibility Points](./device-update-agent-extensibility-points.md#download-handler-extension-type), and the [Microsoft Delta Download Handler runtime deep-dive](./delta-download-handler.md) for how the agent consumes these v5 fields at runtime.

### `relatedFiles` shape and required fields

The `relatedFiles` field on a `files.<id>` entry is a **JSON object keyed by related-file id**, not an array. Each value must include:

| Field | Required by parser | Notes |
|---|---|---|
| `fileName` | yes | Filename used under the sandbox work folder (`workflow_utils.c:503` + `:257-258`). |
| `sizeInBytes` | yes | Validated by content downloader after fetch. |
| `hashes` | yes — missing ⇒ parse fail (`workflow_utils.c:507-512`) | Map of algorithm → hash. The download handler verifies this against the downloaded related-file payload. |
| `properties` | yes — missing ⇒ parse fail (`workflow_utils.c:523-528`) | Free-form map of strings. The download handler implementation decides which property names it consumes — they are opaque to the agent core. |

The related-file id (the object key) **must** appear in the parent update's `fileUrls` map; otherwise parsing fails (`workflow_utils.c:489-499`).

Property names consumed by the Microsoft Delta Download Handler:

| Property name | Required | Used for |
|---|---|---|
| `microsoft.sourceFileHash` | yes | Cache lookup key — must equal the SHA-256 (or other algorithm) of the device's cached source full payload. |
| `microsoft.sourceFileHashAlgorithm` | yes | Cache lookup key — algorithm name, e.g. `"sha256"`. |

Any other property name is informational and not consumed by the handler.

## Multi-Step Ordered Execution (MSOE) Support

Please see the [Multi-Step Update Manifest](https://learn.microsoft.com/en-us/azure/iot-hub-device-update/device-update-multi-step-updates) documentation.

## Parent Update vs. Child Update

The top-level Update Manifest is referred to as the `Parent Update`, and an Update Manifest referenced from a Reference Step is referred to as the `Child Update`.

A `Child Update` must not contain any Reference Steps. This restriction is validated at import time; if violated, the import will fail.

### Inline Step In Parent Update

Inline step(s) specified in `Parent Update` will be applied to the Host Device. Here the ADUC_WorkflowData object that is passed to a Step Handler (aka. Update Content Handler) will not contains a `Selected Components` data. The handler for this type of step should not be a `Component-Aware` handler.

> **Note** | See [Steps Content Handler](../../src/extensions/update_manifest_handlers/steps_handler/README.md) and [Implementing a Component-Aware Step Handler](./how-to-implement-custom-update-handler.md#implementing-a-component-aware-content-handler) for more details.

### Reference Step In Parent Update

Reference step(s) specified in `Parent Update` will be applied to the component on or components connected to the Host Device. A **Reference Step** is a step that contains update identifier of another Update, called `Child Update`. When processing a Reference Step, Steps Handler will download a Detached Update Manifest file specified in the Reference Step data, then validate the file integrity.

Next, the Steps Handler will parse the Child Update Manifest and create ADUC_Workflow object (aka. Child Workflow Data) by combining the data from Child Update Manifest and File URLs information from the Parent Update Manifest. This Child Workflow Data also has a 'level' property set to '1'.

> **Note** | For Update Manifest version v4, the Child Update cannot contain any Reference Steps.

## Detached Update Manifest

To avoid deployment failure due to IoT Hub Twin Data Size Limit, any large Update Manifest will be delivered in a form of JSON data file, called 'Detached Update Manifest'.

If an update with large content is imported into Device Update for IoT Hub, the generated Update Manifest will contain an additional payload file called `Detached Update Manifest` which contains a full data of the Update Manifest.

The `UpdateManifest` property in the Device or Module Twin will contains the Detached Update Manifest file information.

When processing PnP Property Changed Event, Device Update Agent will automatically download the Detached Update Manifest file, and create ADUC_WorkflowData object that contain the full Update Manifest data.
