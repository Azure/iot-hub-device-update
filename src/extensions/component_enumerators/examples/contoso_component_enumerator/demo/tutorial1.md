# TUTORIAL — OTA Multi-Component Update Using APT Handler and Script Handler

This tutorial walks through the structure of an ADU import manifest for a
two-step parent update that targets components attached to a host device
through the `microsoft/contoso-component-enumerator` extension.

> **How this tutorial works.** The JSON below explains the manifest layout
> field-by-field. The `sha256` and `sizeInBytes` values are written as
> placeholders (`"<sha256:…>"`, `"<size>"`) — when authoring your own
> manifest, those must be the real digest and byte length of each payload
> file. For a fully-formed, ready-to-import example that follows the same
> two-step pattern (APT step on host + reference step targeting components),
> see [`sample-updates/Contoso.Virtual-Vacuum.7.0/`](./sample-updates/Contoso.Virtual-Vacuum.7.0/).

## Scenario

Contoso wants to deliver an OTA update to all Virtual Vacuum devices that:

- Installs the latest `tree` Debian package on the host device.
- Installs Virtual Motor firmware version 1.1 on every motor component
  currently connected to the host.

## Device information

| Property      | Value                |
|---------------|----------------------|
| Manufacturer  | **contoso**          |
| Model         | **virtual-vacuum-v1**|

## Update identifier

| Field    | Value             |
|----------|-------------------|
| Provider | **Contoso**       |
| Name     | **Virtual-Vacuum**|
| Version  | **20**            |

## Update types and artifacts

### Host update — APT handler (`microsoft/apt:1`)

- APT manifest for installing `tree`:
  [`apt-manifest-tree-1.0.json`](./sample-updates/data-files/APT/apt-manifest-tree-1.0.json)

### Component (motors) update — Script handler (`microsoft/script:1`)

- Virtual Motor firmware payload:
  [`motor-firmware-1.1.json`](./sample-updates/data-files/motor-firmware-1.1.json)
- Virtual Motor install script:
  [`contoso-motor-installscript.sh`](./sample-updates/scripts/contoso-motor-installscript.sh)

## Preparing the import manifest

> **Prerequisites** | Read [Import an update to Device Update for IoT
> Hub](https://learn.microsoft.com/azure/iot-hub-device-update/import-update)
> and the [import manifest
> schema](https://learn.microsoft.com/azure/iot-hub-device-update/import-schema)
> for the canonical reference. The walk-through below summarizes the schema
> in the context of this tutorial's scenario.

The parent update needs **two top-level steps**:

1. An **inline step** that installs `tree` on the host device (APT handler).
2. A **reference step** that points at a *child update* targeting the motor
   components (script handler).

### Step 1 — Inline APT step on the host

```json
{
  "type": "inline",
  "description": "Install tree Debian package on host device.",
  "handler": "microsoft/apt:1",
  "files": [ "apt-manifest-tree-1.0.json" ],
  "handlerProperties": {
    "installedCriteria": "apt-update-tree-1.0"
  }
}
```

The file referenced in `files[]` must also appear in the manifest's
top-level `files` array with its real SHA-256 hash and byte size:

```json
{
  "filename": "apt-manifest-tree-1.0.json",
  "sizeInBytes": "<size>",
  "hashes": {
    "sha256": "<sha256:apt-manifest-tree-1.0.json>"
  }
}
```

### Step 2 — Child update targeting the `motors` component group

The child update is a **separate, non-deployable** import manifest with
its own update identifier. It is not deployed directly to a device; the
parent update references it by ID, and the agent fans it out to every
component in the targeted group.

The child update identifier and compatibility for this tutorial are:

| Field                      | Value                          |
|----------------------------|--------------------------------|
| Provider                   | **contoso**                    |
| Name                       | **contoso-virtual-motors**     |
| Version                    | **1.1**                        |
| `compatibility[].group`    | **motors**                     |

The child update has one inline script step that copies the firmware file
onto each motor component:

```json
{
  "updateId": {
    "provider": "contoso",
    "name": "contoso-virtual-motors",
    "version": "1.1"
  },
  "isDeployable": false,
  "compatibility": [
    { "group": "motors" }
  ],
  "instructions": {
    "steps": [
      {
        "type": "inline",
        "description": "Motors Update - firmware 1.1 installation",
        "handler": "microsoft/script:1",
        "files": [
          "contoso-motor-installscript.sh",
          "motor-firmware-1.1.json"
        ],
        "handlerProperties": {
          "scriptFileName": "contoso-motor-installscript.sh",
          "installedCriteria": "contoso-contoso-virtual-motors-1.1-step-1",
          "arguments": "--firmware-file motor-firmware-1.1.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path"
        }
      }
    ]
  },
  "files": [
    {
      "filename": "contoso-motor-installscript.sh",
      "sizeInBytes": "<size>",
      "hashes": { "sha256": "<sha256:contoso-motor-installscript.sh>" }
    },
    {
      "filename": "motor-firmware-1.1.json",
      "sizeInBytes": "<size>",
      "hashes": { "sha256": "<sha256:motor-firmware-1.1.json>" }
    }
  ],
  "manifestVersion": "4.0"
}
```

The parent update's reference step then points at this child by ID:

```json
{
  "type": "reference",
  "description": "Motor Firmware 1.1 Update",
  "updateId": {
    "provider": "contoso",
    "name": "contoso-virtual-motors",
    "version": "1.1"
  }
}
```

## Putting it all together — the parent import manifest

```json
{
  "updateId": {
    "provider": "Contoso",
    "name": "Virtual-Vacuum",
    "version": "20"
  },
  "isDeployable": true,
  "compatibility": [
    { "manufacturer": "contoso", "model": "virtual-vacuum-v1" }
  ],
  "instructions": {
    "steps": [
      {
        "type": "inline",
        "description": "Install tree Debian package on host device.",
        "handler": "microsoft/apt:1",
        "files": [ "apt-manifest-tree-1.0.json" ],
        "handlerProperties": {
          "installedCriteria": "apt-update-tree-1.0"
        }
      },
      {
        "type": "reference",
        "description": "Motor Firmware 1.1 Update",
        "updateId": {
          "provider": "contoso",
          "name": "contoso-virtual-motors",
          "version": "1.1"
        }
      }
    ]
  },
  "files": [
    {
      "filename": "apt-manifest-tree-1.0.json",
      "sizeInBytes": "<size>",
      "hashes": { "sha256": "<sha256:apt-manifest-tree-1.0.json>" }
    }
  ],
  "manifestVersion": "4.0"
}
```

When importing, upload **both** import manifests and **all** payload files
referenced in the `files[]` arrays into the Device Update import folder.

## Working example checked into this repo

A fully-formed manifest pair that follows the same parent + child reference
pattern (and includes real `sha256` / `sizeInBytes` values) is available
at [`sample-updates/Contoso.Virtual-Vacuum.7.0/`](./sample-updates/Contoso.Virtual-Vacuum.7.0/).
That example uses APT for the host plus reference steps for `cameras` and
`motors`; you can use it directly to exercise the agent end-to-end without
authoring anything by hand.

## Authoring tools

This tutorial deliberately presents the manifest as plain JSON because
that is the canonical format the import service consumes. To produce the
`sha256` digests and `sizeInBytes` values for your own payloads, use any
SDL-approved hashing tool — for example `sha256sum` on Linux/macOS or
`Get-FileHash -Algorithm SHA256` in PowerShell — or use the official
[Azure CLI Device Update
extension](https://learn.microsoft.com/cli/azure/iot/du/update) which can
generate import manifests for you.

**Congratulations** — at this point you have everything needed to import
the update into the Device Update service.
