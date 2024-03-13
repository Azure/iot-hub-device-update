<#
.SYNOPSIS
Sample script handler for Windows.

.DESCRIPTION
Handles parameter parsing and writing result files. Search for TODO to find necessary changes.

.EXAMPLE
mkdir '/var/lib/adu/downloads/20230130t1143571526'
./workflow-script.ps1 --action-is-installed --work-folder /var/lib/adu/downloads/20230130t1143571526 --result-file /var/lib/adu/downloads/20230130t1143571526/action--action-is-installed_aduc_result.json --installed-criteria 10.0.19041.1865
#>

# ADUC_Result codes. See adu_core.h for a complete list.
$ADUC_Result_Failure = 0
$ADUC_Result_Download_Skipped_FileExists = 502
$ADUC_Result_Install_Success = 600
$ADUC_Result_Install_InProgress = 601
$ADUC_Result_Install_Skipped_UpdateAlreadyInstalled = 603
$ADUC_Result_Install_Skipped_NoMatchingComponents = 604
$ADUC_Result_Install_RequiredImmediateReboot = 605
$ADUC_Result_Install_RequiredReboot = 606
$ADUC_Result_Install_RequiredImmediateAgentRestart = 607
$ADUC_Result_Install_RequiredAgentRestart = 608
$ADUC_Result_Apply_Success = 700
$ADUC_Result_Apply_InProgress = 701
$ADUC_Result_Apply_RequiredImmediateReboot = 705
$ADUC_Result_Apply_RequiredReboot = 706
$ADUC_Result_Apply_RequiredImmediateAgentRestart = 707
$ADUC_Result_Apply_RequiredAgentRestart = 708
$ADUC_Result_Cancel_Success = 800
$ADUC_Result_Cancel_UnableToCancel = 801
$ADUC_Result_IsInstalled_Installed = 900
$ADUC_Result_IsInstalled_NotInstalled = 901

# File that tracks latest installed version.
# TODO: Change this to read from a file version, registry key, etc.
# This example uses local app data folder, e.g. C:\Users\jeffmill\AppData\Local\installed-version.txt
$InstalledVersionFile = Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'installed-version.txt'
function Handle-IsInstalled {
    Param(
        [Parameter(mandatory = $true)][string]$WorkFolder,
        [Parameter(mandatory = $true)][string]$InstalledCriteria
    )
    $resultCode = $ADUC_Result_IsInstalled_NotInstalled

    # TODO: Do IsInstalled check (check file version, registry keys, etc.) and set resultCode accordingly.

    # This sample does a simple string compare.
    $version = Get-Content -LiteralPath $InstalledVersionFile -ErrorAction SilentlyContinue
    if ($version -eq $InstalledCriteria) {
        $resultCode = $ADUC_Result_IsInstalled_Installed
    }

    @{
        'resultCode'         = $resultCode
        'extendedResultCode' = 0
        'resultDetails'      = 'IsInstalled result'
    }
}
function Handle-Download {
    Param(
        [Parameter(mandatory = $true)][string]$WorkFolder,
        [Parameter(mandatory = $true)][string]$InstalledCriteria
    )

    # No way to request a file to be downloaded, as a result, all manifest payload is
    # unconditionally downloaded into sandbox.
    # Task 43114872: Completely Script-based updating feature

    @{
        'resultCode'         = $ADUC_Result_Download_Skipped_FileExists
        'extendedResultCode' = 0
        'resultDetails'      = 'Download_Skipped_FileExists'
    }
}
function Handle-Install {
    Param(
        [Parameter(mandatory = $true)][string]$WorkFolder,
        [Parameter(mandatory = $true)][string]$InstalledCriteria
    )
    # First check if the file is already installed
    $result = Handle-IsInstalled -WorkFolder $WorkFolder -InstalledCriteria $InstalledCriteria
    if ($result.resultCode -eq $ADUC_Result_IsInstalled_Installed) {
        $result = @{
            'resultCode'         = $ADUC_Result_Install_Skipped_UpdateAlreadyInstalled
            'extendedResultCode' = 0
            'resultDetails'      = 'Install_Skipped_UpdateAlreadyInstalled'
        }
    }
    elseif ($result.resultCode -eq $ADUC_Result_Failure) {
        # On failure, pass the is-installed result back unchanged.
    }
    else {
        # TODO: Install and set resultCode accordingly.

        $result = @{
            'resultCode'         = $ADUC_Result_IsInstalled_Installed
            'extendedResultCode' = 0
            'resultDetails'      = 'IsInstalled_Installed'
        }
    }

    $result
}
function Handle-Apply {
    Param(
        [Parameter(mandatory = $true)][string]$WorkFolder,
        [Parameter(mandatory = $true)][string]$InstalledCriteria
    )

    # TODO: Apply and set resultCode accordingly.

    # This sample just writes InstalledCriteria to InstalledVersionFile so that
    # Handle-IsInstalled will think that we're up to date.
    $InstalledCriteria | Out-File -Encoding ascii -LiteralPath $InstalledVersionFile

    @{
        'resultCode'         = $ADUC_Result_Apply_Success
        'extendedResultCode' = 0
        'resultDetails'      = 'Apply_Success'
    }
}
function Handle-Cancel {
    Param(
        [Parameter(mandatory = $true)][string]$WorkFolder,
        [Parameter(mandatory = $true)][string]$InstalledCriteria
    )
    # TODO: Cancel and set resultCode accordingly.

    @{
        'resultCode'         = $ADUC_Result_Cancel_UnableToCancel
        'extendedResultCode' = 0
        'resultDetails'      = 'Cancel_UnableToCancel'
    }
}

#
# main
#

# Arguments parsed from commandline.
$Action = $null
$WorkFolder = $null
$InstalledCriteria = $null
$ResultFile = $null

# longarg-like command-line parsing. (DU doesn't send PowerShell-like Param)
$arg = 0
$successfulParse = $true
while ($successfulParse -and $arg -lt $args.Count) {
    $argv = $args[$arg]
    if ($argv.Length -gt 2 -and $argv.Substring(0, 2) -eq '--') {
        $argv = $argv.Substring(2)
        switch ($argv) {
            'action-apply' { $Action = 'apply' }
            'action-cancel' { $Action = 'cancel' }
            'action-download' { $Action = 'download' }
            'action-install' { $Action = 'install' }
            'action-is-installed' { $Action = 'is-installed' }
            'work-folder' { $arg++; $WorkFolder = $args[$arg] }
            'result-file' { $arg++; $ResultFile = $args[$arg] }
            'installed-criteria' { $arg++; $InstalledCriteria = $args[$arg] }
            default { "Invalid arg: $argv"; $successfulParse = $false }
        }
    }
    else { "Invalid arg: $argv"; $successfulParse = $false }
    $arg++
}
if (!$successfulParse) {
    Write-Warning 'Cannot parse commandline'
    "Args: $args"
    exit 1
}
switch ($Action) {
    'is-installed' {
        $result = Handle-IsInstalled -WorkFolder $WorkFolder -InstalledCriteria $InstalledCriteria
    }
    'download' {
        $result = Handle-Download -WorkFolder $WorkFolder -InstalledCriteria $InstalledCriteria
    }
    'install' {
        $result = Handle-Install -WorkFolder $WorkFolder -InstalledCriteria $InstalledCriteria
    }
    'apply' {
        $result = Handle-Apply -WorkFolder $WorkFolder -InstalledCriteria $InstalledCriteria
    }
    'cancel' {
        $result = Handle-Cancel -WorkFolder $WorkFolder -InstalledCriteria $InstalledCriteria
    }
    default {
        Write-Warning 'Action not handled'
        "Args: $args"
        exit 1
    }
}

# Write ADUC_Result to $ResultFile as JSON.
$result | ConvertTo-Json | Out-File -Encoding ascii $ResultFile

($result.GetEnumerator().ForEach({ "$($_.Name) = $($_.Value)" }) -join '; ')

exit 0

