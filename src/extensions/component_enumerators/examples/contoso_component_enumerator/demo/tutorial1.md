# TUTORIAL - OTA Multi Component Update Using APT Handler and Script Handler

## Scenario

Contoso wants to update all Virtual Vacuum devices by delivering OTA update to accomplish following goals:

- Install the latest `tree` Debian package on the `host` device
- Install the `Virtual Motor` firmware version 1.1 to all Virtual Motors that currently connected to the `host` device

## Device Information

|Property|Value|
|--|---|
|Manufacturer|**contoso**|
|Model|**virtual-vacuum-v1**|

## Update Identifier

At this time, this update is version 20 in their `update line of service`. The following are required values for import manifest creation.

|Field Name|Value|
|--|---|
|Provider|**Contoso**|
|Name|**Virtual-Vacuum**|
|Version| **20** |

## Update Types and Artifacts

### For Host Update

> **NOTE** | The update type for APT Update is **'microsoft/apt:1'**

- APT Manifest file for `tree` installation ([apt-manifest-tree-1.0.json](./sample-updates/data-files/APT/apt-manifest-tree-1.0.json))

### For Component (Motors) Update

> **NOTE** | The update type for Virtual Motor firmware is **'microsoft/script:1'**

- Virtual Motor firmware file ([motor-firmware-1.1.json](./sample-updates/data-files/motor-firmware-1.1.json))
- Virtual Motor firmware installation script ([contoso-motor-installscript.sh](./sample-updates/scripts/contoso-motor-installscript.sh))

> **NOTE** | Save all artifacts listed above to a local folder. E.g., './update-files'

## Preparing Update Instructions (steps)

> **Prerequisites** | Read [Device Update for IoT Hub - Scripts](../../../../../../tools/AduCmdlets/README.md) to understand how to create ADU Import Manifest, and ensure that all required scripts can be run properly.

To accomplish the goals, this update requires 2 `update instruction steps` in the Update Manifest:

1. Install an update for the host device. This step must be a top-level `inline step`.
2. install an update for components (motors) that connected to a host device. This step must be a top-level `reference step`, in order to target the update to certain group of components.

### Top-Level Step #1 - Install `tree` Debian Package

The following command is used to create step #1:

```powershell
# -------------------------------------------------
# Create the first top-level step
# -------------------------------------------------

$topLevelStep1 = New-AduInstallationStep `
                        -Handler 'microsoft/apt:1' `
                        -Files "./update-files/apt-manifest-tree-1.0.json" `
                        -HandlerProperties @{ 'installedCriteria'='apt-update-tree-1.0'} `
                        -Description 'Install tree Debian package on host device.'
```

> **IMPORTANT** | Replace **"./update-files/apt-manifest-tree-1.0.json"** above with the actual file path.

### Top-Level Step #2 - Install `Virtual Motors Firmware 1.0` on all `Motor` components

Since this update is targeting a group of components on the host device, the `child update` and a top-level `reference step` must be created.
The following script snippet is used to create both child update and reference step:

>**NOTE** | Child update identity is required. For this tutorial, we use **"contoso", "contoso-virtual-motors", "1.1"** for `Provider`, `Name`, and `Version` respectively.

```powershell

# -------------------------------------------------
# Create a child update for all 'motor' components
# -------------------------------------------------

$RefUpdateManufacturer = "contoso"
$RefUpdateName = "contoso-virtual-motors"
$RefUpdateVersion = "1.1"

$motorsFirmwareVersion = "1.1"

Write-Host "Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

$motorsUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

# This components update only apply to 'motors' group.
$motorsSelector = @{ group = 'motors' }
$motorsCompat = New-AduUpdateCompatibility  -Properties $motorsSelector

$motorScriptFile = ".\update-files\contoso-motor-installscript.sh"
$motorFirmwareFile = ".\update-files\motor-firmware-$motorsFirmwareVersion.json"

#------------
# ADD STEP(S)
#

# This update contains 1 steps.
$motorsInstallSteps = @()

# Step #1 - install a firmware version 1.1 onto motor component.
$motorsInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files $motorScriptFile, $motorFirmwareFile `
                        -HandlerProperties @{  `
                            'scriptFileName'='contoso-motor-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-1"; `
                            "arguments"="--firmware-file motor-firmware-$motorsFirmwareVersion.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                        }   `
                        `
                        -Description 'Motors Update - firmware 1.1 installation'

# ------------------------------
# Create child update manifest
# ------------------------------

$childUpdateId = $motorsUpdateId
$childUpdateIdStr = "$($childUpdateId.Provider).$($childUpdateId.Name).$($childUpdateId.Version)"
$childPayloadFiles = $motorScriptFile, $motorFirmwareFile
$childCompat = $motorsCompat
$childSteps = $motorsInstallSteps

Write-Host "    Preparing child update manifest $childUpdateIdStr ..."

$childManifest = New-AduImportManifest -UpdateId $childUpdateId -IsDeployable $false `
                                    -Compatibility $childCompat `
                                    -InstallationSteps $childSteps `
                                    -ErrorAction Stop

# Create folder for manifest files and payload.
$outputPath = ".\update-files"
Write-Host "    Saving child manifest files and payload to $outputPath..."
New-Item $outputPath -ItemType Directory -ErrorAction SilentlyContinue | Out-Null

# Generate manifest files.
$childManifest | Out-File "$outputPath\$childUpdateIdStr.importmanifest.json" -Encoding utf8


#
# Create a top-level reference step.
#
$topLevelStep2 = New-AduInstallationStep `
                        -UpdateId $childUpdateId `
                        -Description "Motor Firmware 1.1 Update"
```

## Create an Import Manifest

Create a main import manifest by adding above 2 top-level steps in the main update instructions steps collection:

```powershell
##############################################################
# Update : 20.0
##############################################################

# Update Identity
$UpdateProvider = "Contoso"
$UpdateName = "Virtual-Vacuum"
$UpdateVersion = '20.0'

# Host Device Info
$Manufacturer = "contoso"
$Model = 'virtual-vacuum-v1'

$parentCompat = New-AduUpdateCompatibility -Manufacturer $Manufacturer -Model $Model
$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentUpdateIdStr = "$($parentUpdateId.Provider).$($parentUpdateId.Name).$($parentUpdateId.Version)"


# ------------------------------------------------------
# Create the parent update containing 1 inline step and 1 reference step.
# ------------------------------------------------------
Write-Host "    Preparing parent update $parentUpdateIdStr..."
$payloadFiles =
$parentSteps = @()

    #------------
    # ADD STEP(s)

    # step #1 - Install 'tree' package
    $parentSteps += $topLevelStep1

    # step #2 - Install 'motors firmware'
    $parentSteps += $topLevelStep2

# ------------------------------
# Create parent update manifest
# ------------------------------

Write-Host "    Generating an import manifest $parentUpdateIdStr..."

$parentManifest = New-AduImportManifest -UpdateId $parentUpdateId `
                                    -IsDeployable $true `
                                    -Compatibility $parentCompat `
                                    -InstallationSteps $parentSteps `
                                    -ErrorAction Stop

# Create folder for manifest files and payload.
$outputPath = ".\update-files"
Write-Host "    Saving parent manifest file and payload(s) to $outputPath..."
New-Item $outputPath -ItemType Directory -ErrorAction SilentlyContinue | Out-Null

# Generate manifest file.
$parentManifest | Out-File "$outputPath\$parentUpdateIdStr.importmanifest.json" -Encoding utf8

```

### Putting It All Together

You can find the script for above commands [here (tutorial1.ps1)](./tutorial1.ps1)

**Congratulations**! At this point, you should be ready to import the update into the Device Update service.
