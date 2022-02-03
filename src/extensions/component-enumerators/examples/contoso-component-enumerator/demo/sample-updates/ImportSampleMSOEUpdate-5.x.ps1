#
# Device Update for IoT Hub
# Sample PowerShell script for creating and importing a complex update with mixed installation steps.
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#

#Requires -Version 5.0
#Requires -Modules Az.Accounts, Az.Storage

<#
    .SYNOPSIS
        Create a sample Proxy Update that updates 'motors', 'cameras' and 'steamers' group, and importing it to Device Update for IoT Hub
        through REST API. This sample update contain mock files and cannot be actually installed onto a real device.

    .EXAMPLE
        PS > Import-Module AduImportUpdate.psm1
        PS >
        PS > $BlobContainer = Get-AduAzBlobContainer -SubscriptionId $subscriptionId -ResourceGroupName $resourceGroupName -StorageAccountName $storageAccount -ContainerName $BlobContainerName
        PS >
        PS > $token = Get-MsalToken -ClientId $clientId -TenantId $tenantId -Scopes 'https://api.adu.microsoft.com/user_impersonation' -Authority https://login.microsoftonline.com/$tenantId/v2.0 -Interactive -DeviceCode
        PS >
        PS > ImportSampleMSOEUpdate-5.x.ps1 -AccountEndpoint sampleaccount.api.adu.microsoft.com -InstanceId sampleinstance `
                                            -Container $BlobContainer `
                                            -AuthorizationToken $token
#>
[CmdletBinding()]
Param(
    # ADU account endpoint, e.g. sampleaccount.api.adu.microsoft.com
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AccountEndpoint,

    # ADU Instance Id.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $InstanceId,

    # Azure Storage Blob container for staging update artifacts.
    [Parameter(Mandatory=$true)]
    [ValidateNotNull()]
    [Microsoft.WindowsAzure.Commands.Common.Storage.ResourceModel.AzureStorageContainer] $BlobContainer,

    # Azure Active Directory OAuth authorization token.
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string] $AuthorizationToken,

    # Update Provider
    [ValidateNotNullOrEmpty()]
    [string] $UpdateProvider = "ADU-Client-Eng",

    # Update Provider
    [ValidateNotNullOrEmpty()]
    [string] $UpdateName = "MSOE-Update-Demo",
    
    # Device Manufacturer
    [ValidateNotNullOrEmpty()]
    [string] $DeviceManufacturer = "adu-device",

    # Device Model
    [ValidateNotNullOrEmpty()]
    [string] $DeviceModel = "e2e-test"

)

Import-Module $PSScriptRoot\AduImportUpdate.psm1 -ErrorAction Stop
Import-Module $PSScriptRoot\AduRestApi.psm1 -ErrorAction Stop

$UpdateVersion = '5.0'


# -------------------------------------------------
# Create a child update for all 'motor' components
# -------------------------------------------------

$RefUpdateManufacturer = "contoso"
$RefUpdateName = "virtual-motors"
$RefUpdateVersion = "3.0"

$motorsFirmwareVersion = "1.2"

Write-Host "Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

$motorsUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

# This components update only apply to 'motors' group.
$motorsSelector = @{ group = 'motors' }
$motorsCompat = New-AduUpdateCompatibility  -Properties $motorsSelector

# This update contains 3 steps.
$motorsInstallSteps = @()

# Step #1 - simulating a success pre-install task.
$motorsInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files  `
                            "$PSScriptRoot\scripts\contoso-motor-installscript.sh"  `
                            `
                        -HandlerProperties @{  `
                            'scriptFileName'='contoso-motor-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-0";   `
                             'arguments'="--pre-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                        }   `
                        `
                        -Description 'Motors Update pre-install step'

# Step #2 - install a mock firmware version 1.2 onto motor component.
$motorsInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files  `
                            "$PSScriptRoot\scripts\contoso-motor-installscript.sh", "$PSScriptRoot\data-files\motor-firmware-$motorsFirmwareVersion.json"  `
                            `
                        -HandlerProperties @{  `
                            'scriptFileName'='contoso-motor-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-1"; `
                            "arguments"="--firmware-file motor-firmware-$motorsFirmwareVersion.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                        }   `
                        `
                        -Description 'Motors Update - firmware installation'

# Step #3 - simulating a success post-install task.
$motorsInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files  `
                            "$PSScriptRoot\scripts\contoso-motor-installscript.sh"  `
                            `
                        -HandlerProperties @{  `
                            'scriptFileName'='contoso-motor-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-2"; `
                            "arguments"="--post-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path"  `
                        }   `
                        `
                        -Description 'Motors Update post-install step'

$motorsUpdate = New-AduImportUpdateInput -UpdateId $motorsUpdateId `
                                             -IsDeployable $false `
                                             -Compatibility $motorsCompat `
                                             -InstallationSteps $motorsInstallSteps `
                                             -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference


# --------------------------------------------------
# Create a child update for all 'camera' component.
# --------------------------------------------------

$RefUpdateManufacturer = "contoso"
$RefUpdateName = "virtual-cameras"
$RefUpdateVersion = "3.0"

$camerasFirmwareVersion = "2.1"

Write-Host "Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

$camerasUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

# This components update only apply to 'cameras' group.
$camerasSelector = @{ group = 'cameras' }
$camerasCompat = New-AduUpdateCompatibility  -Properties $camerasSelector

# This update contains 3 steps.
$camerasInstallSteps = @()

# Step #1 - simulating a success pre-install task.
$camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files  `
                            "$PSScriptRoot\scripts\contoso-camera-installscript.sh"  `
                            `
                        -HandlerProperties @{  `
                            'scriptFileName'='contoso-camera-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-0";   `
                             'arguments'="--pre-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                        }   `
                        `
                        -Description 'Cameras Update pre-install step'

# Step #2 - install a mock firmware version 2.1 onto camera component.
$camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files  `
                            "$PSScriptRoot\scripts\contoso-camera-installscript.sh", "$PSScriptRoot\data-files\camera-firmware-$camerasFirmwareVersion.json"  `
                            `
                        -HandlerProperties @{  `
                            'scriptFileName'='contoso-camera-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-1"; `
                            "arguments"="--firmware-file camera-firmware-$camerasFirmwareVersion.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                        }   `
                        `
                        -Description 'Cameras Update - firmware installation'

# Step #3 - simulating a success post-install task.
$camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files  `
                            "$PSScriptRoot\scripts\contoso-camera-installscript.sh"  `
                            `
                        -HandlerProperties @{  `
                            'scriptFileName'='contoso-camera-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-2"; `
                            "arguments"="--post-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path"  `
                        }   `
                        `
                        -Description 'Cameras Update post-install step'

$camerasUpdate = New-AduImportUpdateInput -UpdateId $camerasUpdateId `
                                             -IsDeployable $false `
                                             -Compatibility $camerasCompat `
                                             -InstallationSteps $camerasInstallSteps `
                                             -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference


                                             
# -----------------------------------------------------------
# Create a child update for an optional 'steamer' component
# that currently not connected to the host device.
# -----------------------------------------------------------

$RefUpdateManufacturer = "contoso"
$RefUpdateName = "virtual-steamers"
$RefUpdateVersion = "2.0"

$steamersFirmwareVersion = "2.0"

Write-Host "Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

$steamersUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

# This components update only apply to 'steamers' group.
$steamersSelector = @{ group = 'steamers' }
$steamersCompat = New-AduUpdateCompatibility  -Properties $steamersSelector

# This update contains 3 steps.
$steamersInstallSteps = @()

# Step #1 - simulating a success pre-install task.
$steamersInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files  `
                            "$PSScriptRoot\scripts\contoso-steamer-installscript.sh"  `
                            `
                        -HandlerProperties @{  `
                            'scriptFileName'='contoso-steamer-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-0";   `
                             'arguments'="--pre-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                        }   `
                        `
                        -Description 'Steamers Update pre-install step'

# Step #2 - install a mock firmware version 2.0 onto steamer component.
$steamersInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files  `
                            "$PSScriptRoot\scripts\contoso-steamer-installscript.sh", "$PSScriptRoot\data-files\steamer-firmware-$steamersFirmwareVersion.json"  `
                            `
                        -HandlerProperties @{  `
                            'scriptFileName'='contoso-steamer-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-1"; `
                            "arguments"="--firmware-file steamer-firmware-$steamersFirmwareVersion.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                        }   `
                        `
                        -Description 'Steamers Update - firmware installation'

# Step #3 - simulating a success post-install task.
$steamersInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files  `
                            "$PSScriptRoot\scripts\contoso-steamer-installscript.sh"  `
                            `
                        -HandlerProperties @{  `
                            'scriptFileName'='contoso-steamer-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-2"; `
                            "arguments"="--post-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path"  `
                        }   `
                        `
                        -Description 'Steamers Update post-install step'

$steamersUpdate = New-AduImportUpdateInput -UpdateId $steamersUpdateId `
                                             -IsDeployable $false `
                                             -Compatibility $steamersCompat `
                                             -InstallationSteps $steamersInstallSteps `
                                             -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference



# ------------------------------------------------------------
# Create the parent update which parents the child updates above
# ------------------------------------------------------------
Write-Host "Preparing parent update $UpdateVersion ..."

$parentFile0 = "$PSScriptRoot\data-files\APT\apt-manifest-4.0.json"

$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentCompat = New-AduUpdateCompatibility -DeviceManufacturer $DeviceManufacturer -DeviceModel $DeviceModel
$parentSteps = @()

$parentSteps += New-AduInstallationStep -UpdateId $motorsUpdateId -Description "Motors Firmware Version $motorsFirmwareVersion"
$parentSteps += New-AduInstallationStep -UpdateId $camerasUpdateId -Description "Cameras Firmware Version $camerasFirmwareVersion"
$parentSteps += New-AduInstallationStep -UpdateId $steamersUpdateId -Description "Steamers Firmware Version $steamersFirmwareVersion"

$parentUpdate = New-AduImportUpdateInput -UpdateId $parentUpdateId `
                                         -Compatibility $parentCompat `
                                         -InstallationSteps $parentSteps `
                                         -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference

# -----------------------------------------------------------
# Call ADU Import Update REST API to import the updates above
# -----------------------------------------------------------
Write-Host 'Importing update ...'

$operationId = Start-AduImportUpdate -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                                     -ImportUpdateInput $parentUpdate, $motorsUpdate, $camerasUpdate, $steamersUpdate -ErrorAction Stop -Verbose:$VerbosePreference

Wait-AduUpdateOperation -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                        -OperationId $operationId -Timeout (New-TimeSpan -Minutes 15) -Verbose:$VerbosePreference

