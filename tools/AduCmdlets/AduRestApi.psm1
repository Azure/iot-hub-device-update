#
# Azure Device Update for IoT Hub
# Powershell module for calling Azure Device Update (ADU) REST API.
# Copyright (c) Microsoft Corporation.
#

#Requires -Version 5.0

$API_VERSION = "api-version=2021-06-01-draft"

# --------------------------------------------------------------------------------------------------------------------------------
# INTERNAL METHODS
# --------------------------------------------------------------------------------------------------------------------------------

function Get-AuthorizationHeaders
{
    Param(
        [ValidateNotNullOrEmpty()]
        $AccessToken
    )

    return @{
        'Authorization' = "Bearer $AccessToken"
    }
}

# --------------------------------------------------------------------------------------------------------------------------------
# EXPORTED METHODS
# --------------------------------------------------------------------------------------------------------------------------------

function Start-AduImportUpdate
{
<#
.SYNOPSIS
    Start importing an update to Azure Device Update for IoT Hub. Method returns an operationId to be used for
    polling the operation status.
#>
    [CmdletBinding()]
    Param(
        # ADU account endpoint, e.g. contoso.api.adu.microsoft.com
        [Parameter(Mandatory=$true)]
        [ValidateNotNullOrEmpty()]
        [string] $AccountEndpoint,

        # ADU Instance Id.
        [Parameter(Mandatory=$true)]
        [ValidateNotNullOrEmpty()]
        [string] $InstanceId,

        # Azure Active Directory OAuth Authorization Token.
        [Parameter(Mandatory=$true)]
        [ValidateNotNullOrEmpty()]
        [string] $AuthorizationToken,

        # ADU Import Update API input(s).
        [Parameter(ParameterSetName="Default", Mandatory=$true)]
        [ValidateCount(1, 11)]
        [psobject[]] $ImportUpdateInput,

        # Raw API request body in JSON string format.
        [Parameter(ParameterSetName="Advanced", Mandatory=$true)]
        [ValidateNotNullOrEmpty()]
        [string] $RequestBody
    )

    $url = "https://$AccountEndpoint/deviceupdate/$InstanceId/updates?$API_VERSION&action=import"

    $headers = Get-AuthorizationHeaders -AccessToken $AuthorizationToken
    $headers.Add('Content-Type', 'application/json')

    if ($ImportUpdateInput)
    {
        $RequestBody = $ImportUpdateInput | ConvertTo-Json -Depth 50
    }

    Write-Verbose $RequestBody

    $response = Invoke-WebRequest -Uri $url -Method POST -Headers $headers -Body $RequestBody -Verbose:$VerbosePreference

    if ($response.StatusCode -ne 202)
    {
        Write-Error $response
    }

    # .../updates/operations/86b1c73c-e041-4eea-bc7b-918248ae66da?api-version=2021-06-01-draft
    $operationId = $response.Headers["Operation-Location"].Split('/')[-1].Split('?')[0]
    return $operationId
}

function Get-AduUpdateOperation
{
<#
.SYNOPSIS
    Get Update Operation entity.
#>
    [CmdletBinding()]
    Param(
        # ADU account endpoint, e.g. contoso.api.adu.microsoft.com
        [Parameter(Mandatory=$true)]
        [ValidateNotNullOrEmpty()]
        [string] $AccountEndpoint,

        # ADU Instance Id.
        [Parameter(Mandatory=$true)]
        [ValidateNotNullOrEmpty()]
        [string] $InstanceId,

        # Azure Active Directory OAuth authorization token.
        [Parameter(Mandatory=$true)]
        [ValidateNotNullOrEmpty()]
        [string] $AuthorizationToken,

        # Operation identifier returned by Import/Delete Update API.
        [Parameter(Mandatory=$true)]
        [ValidateNotNullOrEmpty()]
        [string] $OperationId
    )

    $url = "https://$AccountEndpoint/deviceupdate/$InstanceId/updates/operations/$($operationId)?$API_VERSION"
    $headers = Get-AuthorizationHeaders -AccessToken $AuthorizationToken

    $operation = Invoke-RestMethod -Uri $url -Method GET -Headers $headers -Verbose:$VerbosePreference
    return $operation
}

function Wait-AduUpdateOperation
{
<#
.SYNOPSIS
    Poll update operation until completion.
#>
    [CmdletBinding()]
    Param(
        # ADU account endpoint, e.g. contoso.api.adu.microsoft.com
        [Parameter(Mandatory=$true)]
        [ValidateNotNullOrEmpty()]
        [string] $AccountEndpoint,

        # ADU Instance Id.
        [Parameter(Mandatory=$true)]
        [ValidateNotNullOrEmpty()]
        [string] $InstanceId,

        # Azure Active Directory OAuth authorization token.
        [Parameter(Mandatory=$true)]
        [ValidateNotNullOrEmpty()]
        [string] $AuthorizationToken,

        # Operation identifier returned by Import/Delete Update API.
        [Parameter(Mandatory=$true)]
        [ValidateNotNullOrEmpty()]
        [string] $OperationId,

        # How long to wait before timing out.
        [timespan] $Timeout = [timespan]::FromMinutes(30)
    )

    Write-Host "Waiting for import operation $operationId to complete " -NoNewline

    if ($VerbosePreference)
    {
        Write-Host '...'
    }

    $endTime = (Get-Date).Add($Timeout)

    do
    {
        $operation = Get-AduUpdateOperation -AccountEndpoint $AccountEndpoint -InstanceId $InstanceId -AuthorizationToken $token -OperationId $operationId -ErrorAction Stop

        if ($VerbosePreference)
        {
            Write-Verbose "Operation status: $($operation.Status), TraceId: $($operation.TraceId)"
        }
        else
        {
            Write-Host '.' -NoNewline
        }

        if ($operation.Status -eq 'Failed' -or $operation.Status -eq 'Succeeded')
        {
            break;
        }

        Start-Sleep -Seconds 15

    } while ((Get-Date) -lt $endTime)

    Write-Host ''

    $operation | ConvertTo-Json -Depth 20 | Out-Host

    if ($operation.Status -ne 'Succeeded')
    {
        Write-Error "Failed to import update."
    }
    else
    {
        Write-Host "Update imported successfully." -ForegroundColor Green
    }
}

Export-ModuleMember -Function Start-AduImportUpdate, Get-AduUpdateOperation, Wait-AduUpdateOperation
