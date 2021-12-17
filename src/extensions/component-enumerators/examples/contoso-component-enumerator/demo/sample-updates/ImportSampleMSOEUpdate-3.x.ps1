#
# Device Update for IoT Hub
# Sample PowerShell script for creating and importing a complex update with failure in child updates' steps.
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#

#Requires -Version 5.0
#Requires -Modules Az.Accounts, Az.Storage

<#
    .SYNOPSIS
        Create a sample updates with failure in various steps in child updates, and importing it to Device Update for IoT Hub
        through REST API. This sample update contain fake files and cannot be actually installed onto a real device.

    .EXAMPLE
        PS > Import-Module AduImportUpdate.psm1
        PS >
        PS > $BlobContainer = Get-AduAzBlobContainer -SubscriptionId $subscriptionId -ResourceGroupName $resourceGroupName -StorageAccountName $storageAccount -ContainerName $BlobContainerName
        PS >
        PS > $token = Get-MsalToken -ClientId $clientId -TenantId $tenantId -Scopes 'https://api.adu.microsoft.com/user_impersonation' -Authority https://login.microsoftonline.com/$tenantId/v2.0 -Interactive -DeviceCode
        PS >
        PS > ImportSampleMSOEUpdate-3.x.ps1 -AccountEndpoint sampleaccount.api.adu.microsoft.com -InstanceId sampleinstance `
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

$UpdateVersion = '3.0'

# --------------------------------------------------
# Create a child update for all 'camera' component.
# --------------------------------------------------

$RefUpdateManufacturer = "contoso"
$RefUpdateName = "virtual-camera"
$RefUpdateVersion = "1.1"

Write-Host "Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

$camerasUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

# This components update only apply to 'cameras' group.
$camerasSelector = @{ group = 'cameras' }
$camerasCompat = New-AduUpdateCompatibility  -Properties $camerasSelector

# This update contains 3 steps.
$camerasInstallSteps = @()

# Step #1 - simulating a failure while performing pre-install task.
$camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files  `
                            "$PSScriptRoot\scripts\contoso-camera-installscript.sh"  `
                            `
                        -HandlerProperties @{  `
                            'scriptFileName'='missing-contoso-camera-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-0";   `
                             'arguments'="--pre-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                        }   `
                        `
                        -Description 'Cameras Update - pre-install step (failure - missing file)'

# Step #2 - install a mock firmware version 1.1 onto camera component.
$camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files  `
                            "$PSScriptRoot\scripts\contoso-camera-installscript.sh", "$PSScriptRoot\data-files\camera-firmware-1.1.json"  `
                            `
                        -HandlerProperties @{  `
                            'scriptFileName'='contoso-camera-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-1"; `
                            "arguments"="--firmware-file camera-firmware-1.1.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
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
                        -Description 'Cameras Update - post-install script'

$camerasUpdate = New-AduImportUpdateInput -UpdateId $camerasUpdateId `
                                             -IsDeployable $false `
                                             -Compatibility $camerasCompat `
                                             -InstallationSteps $camerasInstallSteps `
                                             -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference


# ------------------------------------------------------------
# Create the parent update which parents the child update above
# ------------------------------------------------------------
Write-Host "Preparing parent update $UpdateVersion ..."


$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentCompat = New-AduUpdateCompatibility -DeviceManufacturer $DeviceManufacturer -DeviceModel $DeviceModel
$parentSteps = @()
$parentSteps += New-AduInstallationStep -UpdateId $camerasUpdateId -Description "Cameras Firmware Update"

$parentUpdate = New-AduImportUpdateInput -UpdateId $parentUpdateId `
                                         -Compatibility $parentCompat `
                                         -InstallationSteps $parentSteps `
                                         -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference

# -----------------------------------------------------------
# Call ADU Import Update REST API to import the updates above
# -----------------------------------------------------------
Write-Host 'Importing update ...'

$operationId = Start-AduImportUpdate -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                                     -ImportUpdateInput $parentUpdate, $camerasUpdate -ErrorAction Stop -Verbose:$VerbosePreference

Wait-AduUpdateOperation -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                        -OperationId $operationId -Timeout (New-TimeSpan -Minutes 15) -Verbose:$VerbosePreference


# -----------------------------------------------------------
# Failure case #2 - reference update #1 step #2 failed
# -----------------------------------------------------------
$UpdateVersion = '3.1'

$RefUpdateVersion = "1.2"

Write-Host "Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

$camerasUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

# This components update only apply to 'cameras' group.
$camerasSelector = @{ group = 'cameras' }
$camerasCompat = New-AduUpdateCompatibility  -Properties $camerasSelector

# This update contains 3 steps
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
                        -Description 'Cameras Update - pre-install step'

# Step #2 - Simulating a failure while installing a mock firmware version 1.2 onto camera component.
$camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files  `
                            "$PSScriptRoot\scripts\contoso-camera-installscript.sh", "$PSScriptRoot\data-files\camera-firmware-1.1.json"  `
                            `
                        -HandlerProperties @{  `
                            'scriptFileName'='missing-contoso-camera-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-1"; `
                            "arguments"="--firmware-file camera-firmware-1.1.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                        }   `
                        `
                        -Description 'Cameras Update - firmware installation (failure - missing file)'

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
                        -Description 'Cameras Update - post-install step'

$camerasUpdate = New-AduImportUpdateInput -UpdateId $camerasUpdateId `
                                             -IsDeployable $false `
                                             -Compatibility $camerasCompat `
                                             -InstallationSteps $camerasInstallSteps `
                                             -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference


# ------------------------------------------------------------
# Create the parent update which parents the child update above
# ------------------------------------------------------------
Write-Host "Preparing parent update $UpdateVersion ..."

$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentCompat = New-AduUpdateCompatibility -DeviceManufacturer $DeviceManufacturer -DeviceModel $DeviceModel
$parentSteps = @()
$parentSteps += New-AduInstallationStep -UpdateId $camerasUpdateId -Description "Cameras Firmware Update"

$parentUpdate = New-AduImportUpdateInput -UpdateId $parentUpdateId `
                                         -Compatibility $parentCompat `
                                         -InstallationSteps $parentSteps `
                                         -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference

# -----------------------------------------------------------
# Call ADU Import Update REST API to import the updates above
# -----------------------------------------------------------
Write-Host 'Importing update ...'

$operationId = Start-AduImportUpdate -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                                     -ImportUpdateInput $parentUpdate, $camerasUpdate -ErrorAction Stop -Verbose:$VerbosePreference

Wait-AduUpdateOperation -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                        -OperationId $operationId -Timeout (New-TimeSpan -Minutes 15) -Verbose:$VerbosePreference


# -----------------------------------------------------------
# Failure case #3 - reference update #1 step #3 failed
# -----------------------------------------------------------
$UpdateVersion = '3.2'

$RefUpdateVersion = "1.3"

Write-Host "Preparing child update ($RefUpdateManufacturer/$RefUpdateName/$RefUpdateVersion)..."

$camerasUpdateId = New-AduUpdateId -Provider $RefUpdateManufacturer -Name $RefUpdateName -Version $RefUpdateVersion

# This components update only apply to 'cameras' group.
$camerasSelector = @{ group = 'cameras' }
$camerasCompat = New-AduUpdateCompatibility  -Properties $camerasSelector


# This update contains 3 steps
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
                        -Description 'Cameras Update - pre-install step'

# Step #2 - install a mock firmware version 1.3 onto camera component.
$camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files  `
                            "$PSScriptRoot\scripts\contoso-camera-installscript.sh", "$PSScriptRoot\data-files\camera-firmware-1.1.json"  `
                            `
                        -HandlerProperties @{  `
                            'scriptFileName'='contoso-camera-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-1"; `
                            "arguments"="--firmware-file camera-firmware-1.1.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
                        }   `
                        `
                        -Description 'Cameras Update - firmware installation'

# Step #3 - simulating a failure in post-install task.
$camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files  `
                            "$PSScriptRoot\scripts\contoso-camera-installscript.sh"  `
                            `
                        -HandlerProperties @{  `
                            'scriptFileName'='missing-contoso-camera-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-2"; `
                            "arguments"="--post-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path"  `
                        }   `
                        `
                        -Description 'Cameras Update - post-install step (failure - missing file)'

$camerasUpdate = New-AduImportUpdateInput -UpdateId $camerasUpdateId `
                                             -IsDeployable $false `
                                             -Compatibility $camerasCompat `
                                             -InstallationSteps $camerasInstallSteps `
                                             -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference


# ------------------------------------------------------------
# Create the parent update which parents the child update above
# ------------------------------------------------------------
Write-Host "Preparing parent update $UpdateVersion ..."

$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentCompat = New-AduUpdateCompatibility -DeviceManufacturer $DeviceManufacturer -DeviceModel $DeviceModel
$parentSteps = @()
$parentSteps += New-AduInstallationStep -UpdateId $camerasUpdateId -Description "Cameras Firmware Update"

$parentUpdate = New-AduImportUpdateInput -UpdateId $parentUpdateId `
                                         -Compatibility $parentCompat `
                                         -InstallationSteps $parentSteps `
                                         -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference

# -----------------------------------------------------------
# Call ADU Import Update REST API to import the updates above
# -----------------------------------------------------------
Write-Host 'Importing update ...'

$operationId = Start-AduImportUpdate -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                                     -ImportUpdateInput $parentUpdate, $camerasUpdate -ErrorAction Stop -Verbose:$VerbosePreference

Wait-AduUpdateOperation -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                        -OperationId $operationId -Timeout (New-TimeSpan -Minutes 15) -Verbose:$VerbosePreference


                        
# -----------------------------------------------------------
# Success case
# -----------------------------------------------------------
$UpdateVersion = '3.3'

$RefUpdateVersion = "1.4"

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
                        -Description 'Cameras Update - pre-install step'

# Step #2 - install a mock firmware version 1.4 onto camera component.
$camerasInstallSteps += New-AduInstallationStep -Handler 'microsoft/script:1' `
                        -Files  `
                            "$PSScriptRoot\scripts\contoso-camera-installscript.sh", "$PSScriptRoot\data-files\camera-firmware-1.1.json"  `
                            `
                        -HandlerProperties @{  `
                            'scriptFileName'='contoso-camera-installscript.sh';  `
                            'installedCriteria'="$RefUpdateManufacturer-$RefUpdateName-$RefUpdateVersion-step-1"; `
                            "arguments"="--firmware-file camera-firmware-1.1.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path" `
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
                        -Description 'Cameras Update - post-install step'

$camerasUpdate = New-AduImportUpdateInput -UpdateId $camerasUpdateId `
                                             -IsDeployable $false `
                                             -Compatibility $camerasCompat `
                                             -InstallationSteps $camerasInstallSteps `
                                             -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference


# ------------------------------------------------------------
# Create the parent update which parents the child update above
# ------------------------------------------------------------
Write-Host "Preparing parent update $UpdateVersion ..."


$parentUpdateId = New-AduUpdateId -Provider $UpdateProvider -Name $UpdateName -Version $UpdateVersion
$parentCompat = New-AduUpdateCompatibility -DeviceManufacturer $DeviceManufacturer -DeviceModel $DeviceModel
$parentSteps = @()
$parentSteps += New-AduInstallationStep -UpdateId $camerasUpdateId -Description "Cameras Firmware Update"

$parentUpdate = New-AduImportUpdateInput -UpdateId $parentUpdateId `
                                         -Compatibility $parentCompat `
                                         -InstallationSteps $parentSteps `
                                         -BlobContainer $BlobContainer -ErrorAction Stop -Verbose:$VerbosePreference

# -----------------------------------------------------------
# Call ADU Import Update REST API to import the updates above
# -----------------------------------------------------------
Write-Host 'Importing update ...'

$operationId = Start-AduImportUpdate -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                                     -ImportUpdateInput $parentUpdate, $camerasUpdate -ErrorAction Stop -Verbose:$VerbosePreference

Wait-AduUpdateOperation -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $AuthorizationToken `
                        -OperationId $operationId -Timeout (New-TimeSpan -Minutes 15) -Verbose:$VerbosePreference

