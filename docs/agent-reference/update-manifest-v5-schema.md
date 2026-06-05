# Update Manifest Version 5

The Update Manifest is the JSON document the Device Update service delivers to a device to describe one deployment: what update it is, what files make it up, and what steps the agent must execute. This page is the device-side reference for how a v5 manifest is shaped, how it is wrapped on the wire, and how the agent consumes each field at runtime.

For the cloud-side authoring view (what you submit at import time), see the public [Import Manifest](https://learn.microsoft.com/azure/iot-hub-device-update/import-schema) docs on Microsoft Learn.

## Table of Contents

- [Schema](#schema)
  - [Official JSON Schemas](#official-json-schemas)
  - [What changed between v4 and v5](#what-changed-between-v4-and-v5)
- [Delivery and framing](#delivery-and-framing)
  - [Twin delivery and the Detached Update Manifest](#twin-delivery-and-the-detached-update-manifest)
  - [Parent Update, Child Update, and Multi-Step Ordered Execution](#parent-update-child-update-and-multi-step-ordered-execution)
- [Field → Agent Consumption Map](#field--agent-consumption-map)
  - [Twin envelope (desired properties on the `deviceUpdate` PnP component)](#twin-envelope-desired-properties-on-the-deviceupdate-pnp-component)
  - [Manifest body (inside the signed `updateManifest` JWS payload)](#manifest-body-inside-the-signed-updatemanifest-jws-payload)
  - [`files.<id>.relatedFiles` deep-dive](#filesidrelatedfiles-deep-dive)

---

## Schema

### Official JSON Schemas

The Azure Device Update service publishes JSON Schemas for the **import manifest** (the document you author and submit to Device Update for IoT Hub) on [SchemaStore](https://www.schemastore.org/azure-deviceupdate-update-manifest-5.json):

| Version | JSON Schema URL | |
| ------- | --------------- | ------------------------- |
| v4 (`"manifestVersion": "4.0"`) | <https://json.schemastore.org/azure-deviceupdate-import-manifest-4.0.json> |
| v5 (`"manifestVersion": "5.0"`) | <https://json.schemastore.org/azure-deviceupdate-import-manifest-5.0.json> |

Reference one of these from the `$schema` property of your import manifest to get editor validation and IntelliSense:

```json
{
  "$schema": "https://json.schemastore.org/azure-deviceupdate-import-manifest-5.0.json",
  "manifestVersion": "5.0",
  ...
}
```

> **Note.** The schemas above describe the **import manifest** that you submit to the Device Update service. The service then synthesizes the **update manifest** that is delivered to devices via the IoT Hub twin. The two formats share the same core shape, but the update manifest also carries service-generated fields (e.g. signed JWS, file URLs).

### What changed between v4 and v5

The structural definitions for `updateId`, `compatibility`, `instructions`, `inlineStep`, and `referenceStep` are unchanged between v4 and v5. The differences are confined to the per-file `files` entries and enable the **Download Handler** extensibility point:

| Capability | v4 | v5 |
| ---------- | -- | -- |
| `manifestVersion` | `"4.0"` | `"5.0"` |
| `files[].downloadHandler` (object with `id`, e.g. `"microsoft/delta:1"`) | ❌ Not supported | ✅ Optional — opts a payload file into a registered Download Handler so the agent can produce the payload from another source instead of downloading it in full |
| `files[].relatedFiles` (**JSON object/map** keyed by related-file id; each value has its own `fileName`, `sizeInBytes`, `hashes`, and **required** freeform `properties`) | ❌ Not supported | ✅ Optional — declares additional payloads consumed by the download handler (for example, the delta file plus `microsoft.sourceFileHashAlgorithm` / `microsoft.sourceFileHash` properties used by the [Microsoft Delta Download Handler](./delta-download-handler.md)) |

In short, **v5 = v4 plus the ability to attach a Download Handler and `relatedFiles` to any payload file**. Existing v4 manifests remain valid as v5 manifests after bumping `manifestVersion` to `"5.0"`.

See [Download Handler Extensibility Point](../../src/extensions/download_handlers/README.md), [Device Update Agent Extensibility Points](./device-update-agent-extensibility-points.md#download-handler-extension-type), and the [Microsoft Delta Download Handler runtime deep-dive](./delta-download-handler.md) for how the agent consumes these v5 fields at runtime.

---

## Delivery and framing

### Twin delivery and the Detached Update Manifest

The agent receives an update manifest as a **desired property on the `deviceUpdate` PnP component**. The desired-property payload (the [Twin envelope](#twin-envelope-desired-properties-on-the-deviceupdate-pnp-component)) carries the signed JWS body in its `updateManifest` field, plus signature, file URLs, and root-key package URL.

To avoid deployment failure due to the IoT Hub twin data-size limit, **large** Update Manifests are not embedded directly. The service instead emits a small twin payload that contains an additional payload file called the **Detached Update Manifest**, and the `UpdateManifest` twin property carries the file information rather than inline JSON.

When processing the PnP property-changed event, the Device Update Agent will automatically download the Detached Update Manifest file and create the in-memory `ADUC_WorkflowData` object that contains the full Update Manifest data. After this point, the rest of the workflow is identical whether the manifest was delivered inline or detached.

### Parent Update, Child Update, and Multi-Step Ordered Execution

The top-level Update Manifest is referred to as the **Parent Update**, and an Update Manifest referenced from a Reference Step is referred to as a **Child Update**. A Child Update **must not** contain any further Reference Steps; this restriction is validated at import time and the import will fail if violated.

A v5 manifest's `instructions.steps[]` array drives Multi-Step Ordered Execution (MSOE). For the cloud-side reference see the public [Multi-Step Update Manifest](https://learn.microsoft.com/azure/iot-hub-device-update/device-update-multi-step-updates) docs; the device-side semantics are summarized below and detailed (per phase) in [steps-handler.md](steps-handler.md).

#### Inline Step in a Parent Update

An inline step in the Parent Update applies to the **Host Device** itself. The `ADUC_WorkflowData` object passed to the Step Handler does **not** contain `Selected Components` data, so the handler chosen by `instructions.steps[].handler` should not be a `Component-Aware` handler.

> **Note.** See [Steps Content Handler](../../src/extensions/update_manifest_handlers/steps_handler/README.md) and [Implementing a Component-Aware Step Handler](./how-to-implement-custom-update-handler.md#implementing-a-component-aware-content-handler) for details.

#### Reference Step in a Parent Update

A reference step in the Parent Update applies to a component on, or components connected to, the Host Device. A **Reference Step** is a step whose `instructions.steps[].updateId` identifies another Update — the **Child Update**. When the Steps Handler encounters a reference step it:

1. Downloads the Detached Update Manifest file identified by the step's `detachedManifestFileId`.
2. Validates the file's integrity (hash + signature chain).
3. Parses the Child Update Manifest and creates a child `ADUC_WorkflowData` object by combining the Child Update Manifest body with the **File URLs** carried by the Parent Update Manifest. The child workflow has its `level` property set to `1`.
4. Recurses one level (no further nesting permitted, per the Child-Update rule above).

> **Note.** For Update Manifest v4, the Child Update cannot contain any Reference Steps either. This is unchanged in v5.

---

## Field → Agent Consumption Map

A v5 update manifest is delivered to the device as a **signed JWS** sitting inside the IoT Hub device-twin desired properties. The agent verifies the signature, parses the payload, and walks each field through one or more workflow phases. For the high-level workflow that consumes these fields end-to-end, see [architecture-overview.md](./architecture-overview.md#end-to-end-flow).

### Twin envelope (desired properties on the `deviceUpdate` PnP component)

| Field | Purpose | Parsed by | Workflow phase that consumes it | If absent / invalid |
|---|---|---|---|---|
| `workflow.action` | `3 = ProcessDeployment`, `255 = Cancel` | `workflow_parse_peek_unprotected_workflow_properties` (`workflow_utils.c`) | Read before any phase; chooses `HandleUpdateAction` branch | `Undefined` → ignored |
| `workflow.id` | Service-assigned deployment id. Special value `"nodeployment"` paired with `Cancel` means "no work for this device group" | same | Echoed back in every reported `workflow.id` | Cancel + `"nodeployment"` is silently dropped (`adu_core_interface.c`) |
| `workflow.retryTimestamp` | Service-supplied retry token. A change in this value enables same-workflow retry processing via `AgentOrchestration_IsRetryApplicable` / `workflow_update_retry_deployment` | same | `ADUC_Workflow_HandlePropertyUpdate` re-runs the workflow when the token changes | Optional |
| `updateManifest` | JSON **string** containing the v5 manifest body (or a pointer to a Detached Update Manifest file when the body is too large for the twin) | `workflow_parse` | All phases parse fields from this object | Parse failure ⇒ `Failed` |
| `updateManifestSignature` | JWS whose signed payload is the SHA-256 hash of `updateManifest`. Verified by `jws_utils.c` (`VerifyJWSWithSJWK` / `VerifyJWSWithKey`); the hash check is in `workflow_utils.c` | `jws_utils.c`, `workflow_utils.c` | Verified before any download | Verification failure ⇒ `Failed`, no content fetched |
| `fileUrls` | Map of `fileId` → download URL (HTTP or HTTPS — the agent does not enforce the scheme) | `workflow_get_entity_workfolder_filepath`, `extension_manager.cpp` | Download phase, per file entity | Missing URL for required file ⇒ download failure |
| `rootKeyPackageUrl` | URL to the signed root-key package | `rootkey_workflow` | Run before manifest signature verification | Failure ⇒ continues with on-disk root keys (best-effort) |

### Manifest body (inside the signed `updateManifest` JWS payload)

| Field | Purpose | Parsed by | Workflow phase that consumes it | If absent / invalid |
|---|---|---|---|---|
| `manifestVersion` | `"4.0"` or `"5.0"` | `workflow_get_update_manifest_version` | Selects Update Manifest Handler `microsoft/update-manifest:<n>`; falls back to default `microsoft/update-manifest` if the versioned variant fails to load (`linux_adu_core_impl.cpp`) | Unsupported ⇒ no handler ⇒ `Failed` |
| `updateId.{provider,name,version}` | Globally unique update identity (`ADUC_UpdateId`) | `workflow_get_expected_update_id` | Reported as `installedUpdateId` **only after** successful Apply (`SetInstalledUpdateIdAndGoToIdle`) | Required; missing ⇒ parse failure |
| `compatibility[]` | Service uses this for targeting; agent uses it (child manifest only) for component selection through the registered Component Enumerator | `workflow_get_compatibility` | Reference-step processing (level 1) only | Level 0: not consumed at runtime |
| `instructions.steps[]` | Ordered list of inline / reference steps. Top-level update has no `updateType`; agent implicitly uses `microsoft/steps:1` | `workflow_get_instructions_steps_count`, `workflow_get_step` | Iterated in Download / Install (per Steps Handler) | Empty ⇒ nothing to do; not currently treated as failure |
| `instructions.steps[].type` | `"inline"` (default) or `"reference"` | `workflow_peek_step_type` | Steps Handler chooses inline-handler load vs detached-manifest download + recursion | Missing ⇒ defaults to `"reference"` in `workflow_peek_step_type` but `workflow_is_inline_step` treats anything not `"reference"` as inline — be explicit |
| `instructions.steps[].handler` (inline) | e.g. `microsoft/swupdate:2`, `microsoft/apt:1`, `microsoft/script:1` | same | Inline step → `LoadUpdateContentHandlerExtension(handler)` then run handler's `IsInstalled` → `Download` → `Backup` → `Install` → `Apply` | Missing for inline step ⇒ load failure ⇒ `Failed` |
| `instructions.steps[].handlerProperties` | Free-form bag of args forwarded to the step handler (e.g. installedCriteria, scriptFileName, arguments) | `workflow_peek_step_handler_property` | Read by the **selected step handler** (not by Steps Handler itself) | Handler-specific |
| `instructions.steps[].files[]` | File IDs from the parent `files` map that this step needs | `PrepareStepsWorkflowDataObject` | Inline-step child workflow is created with this **subset** of file entities | Empty ⇒ no payload for the step |
| `instructions.steps[].updateId` (reference) | Identifies the **child** update to recurse into | `workflow_get_update_id` (child manifest) | Reference step → triggers detached-manifest download and child workflow | Required for reference steps |
| `instructions.steps[].detachedManifestFileId` (reference) | `fileId` of the child manifest payload | `workflow_get_step_detached_manifest_file` | Steps Handler downloads + verifies the child manifest before recursing | Required for reference steps |
| `files` (map of `fileId` → entry) | Payload index | `workflow_get_update_file`, `workflow_get_update_files_count` | Download phase enumerates this map | Empty / id mismatch ⇒ download failure |
| `files.<id>.fileName` | File name to use under the sandbox work folder | same | Download phase | Required |
| `files.<id>.sizeInBytes` | Expected file size | same | Validated by content downloader after fetch | Mismatch ⇒ `Failed` |
| `files.<id>.hashes` (e.g. `sha256`) | Map of algorithm → hash | `parser_utils.c`, `hash_utils.c` | Hash check after download (full or delta-reconstructed) | Mismatch ⇒ `Failed` |
| `files.<id>.arguments` | Per-file arguments forwarded to handler | `workflow_get_update_file` (sets `ADUC_FileEntity.Arguments`) | Step handler specific | Optional |
| `files.<id>.relatedFiles` (**v5**) | Map of `relatedFileId` → entry (auxiliary payloads consumed by a Download Handler — e.g. delta source-file metadata + hashes). Parsed only when the file entity declares a `downloadHandler`. See [deep-dive](#filesidrelatedfiles-deep-dive) below | `workflow_get_related_files` | Download Handler phase only | If a `downloadHandler` references missing `relatedFiles` ⇒ Download Handler fails ⇒ fall back to full download |
| `files.<id>.relatedFiles.<rid>.properties` (**v5**) | Free-form map (e.g. `microsoft.sourceFileHashAlgorithm`, `microsoft.sourceFileHash` for Microsoft Delta). **Required** by `workflow_utils.c` when a related file is present | same | Read by the Download Handler implementation | Parse failure on missing `properties` |
| `files.<id>.downloadHandler.id` (**v5**) | Identifies a registered Download Handler extension (e.g. `microsoft/delta:1`) | `parser_utils.c` (sets `ADUC_FileEntity.DownloadHandlerId`) | `ExtensionManager::Download` attempts the Download Handler **first**; on `ADUC_Result_Download_Handler_RequiredFullDownload` (or any download-handler failure) it falls back to the standard Content Downloader | Unknown id ⇒ fall back to full download |
| `createdDateTime`, `mimeType` | Service / schema metadata | parsed but not used in the runtime workflow | — | Not enforced |

> The Steps Handler builds a **tree** of `ADUC_WorkflowHandle` objects: the parent at level 0, one child per top-level step at level 1, and (for reference steps only) grandchild steps at level 2. The same seven `ContentHandler` virtual methods (`IsInstalled`, `Download`, `Backup`, `Install`, `Apply`, `Restore`, `Cancel`) are dispatched at every level; the `.so` itself only exports the factory symbol `CreateUpdateContentHandlerExtension`. See [steps-handler.md](steps-handler.md) for the full phase-by-phase contract.

### `files.<id>.relatedFiles` deep-dive

The `relatedFiles` field on a `files.<id>` entry is a **JSON object keyed by related-file id**, not an array. Each value must include:

| Field | Required by parser | Notes |
|---|---|---|
| `fileName` | yes | Filename used under the sandbox work folder. Parsed by `workflow_parse_related_file` (`workflow_utils.c`). |
| `sizeInBytes` | yes | Validated by content downloader after fetch. |
| `hashes` | yes — missing ⇒ parse fail (`workflow_utils.c`, `workflow_parse_related_file`) | Map of algorithm → hash. The download handler verifies this against the downloaded related-file payload. |
| `properties` | yes — missing ⇒ parse fail (`workflow_utils.c`, `workflow_parse_related_file`) | Free-form map of strings. The download handler implementation decides which property names it consumes — they are opaque to the agent core. |

The related-file id (the object key) **must** appear in the parent update's `fileUrls` map; otherwise parsing fails (`workflow_utils.c`, `workflow_parse_related_file`).

Property names consumed by the Microsoft Delta Download Handler:

| Property name | Required | Used for |
|---|---|---|
| `microsoft.sourceFileHash` | yes | Cache lookup key — must equal the SHA-256 (or other algorithm) of the device's cached source full payload. |
| `microsoft.sourceFileHashAlgorithm` | yes | Cache lookup key — algorithm name, e.g. `"sha256"`. |

Any other property name is informational and not consumed by the handler.
