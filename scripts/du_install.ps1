<#
.SYNOPSIS
Locally install / Package the Device Update product.

.DESCRIPTION
Assumes the product has been built locally using build.ps1

.EXAMPLE
PS> .\scripts\du_install.ps1 -Type Debug -Install -Package
#>

Param(
    # Output directory. Default is {git_root}/out
    [string]$BuildOutputPath,
    # Build type
    [ValidateSet('Release', 'RelWithDebInfo', 'Debug')]
    [string]$Type = 'Debug',
    # Build package?
    [switch]$Package = $false
)

function Show-Warning {
    Param([Parameter(mandatory = $true, position = 0)][string]$Message)
    Write-Host -ForegroundColor Yellow -NoNewline 'Warning:'
    Write-Host " $Message"
}

function Show-Error {
    Param([Parameter(mandatory = $true, position = 0)][string]$Message)
    Write-Host -ForegroundColor Red -NoNewline 'Error:'
    Write-Host " $Message"
}

function Show-Header {
    Param([Parameter(mandatory = $true, position = 0)][string]$Message)
    $sep = ":{0}:" -f (' ' * ($Message.Length + 2))
    Write-Host -ForegroundColor DarkYellow -BackgroundColor DarkBlue $sep
    Write-Host -ForegroundColor White -BackgroundColor DarkBlue ("  {0}  " -f $Message)
    Write-Host -ForegroundColor DarkYellow -BackgroundColor DarkBlue $sep
    ''
}

function Show-Bullet {
    Param([Parameter(mandatory = $true, position = 0)][string]$Message)
    Write-Host -ForegroundColor Blue -NoNewline '*'
    Write-Host " $Message"
}

function Invoke-CopyFile {
    Param(
        [Parameter(mandatory = $true)][string]$Source,
        [Parameter(mandatory = $true)][string]$Destination)

    if (-not (Test-Path -LiteralPath $Source -PathType Leaf)) {
        throw "$Source is not a file or doesn't exist"
    }

    if (-not (Test-Path -LiteralPath $Destination -PathType Container)) {
        throw "$Destination is not a folder"
    }

    $copyNeeded = $true

    $destinationFile = Join-Path $Destination (Split-Path $Source -Leaf)
    if (Test-Path $destinationFile -PathType Leaf) {
        $d_lwt = (Get-ChildItem $destinationFile).LastWriteTime
        $s_lwt = (Get-ChildItem $Source).LastWriteTime

        if ($d_lwt -ge $s_lwt) {
            # Destination is up to date or newer
            $copyNeeded = $false
        }
    }

    if ($copyNeeded) {
        "Copied: $Source => $Destination"
        # Force, in case destination is marked read-only
        Copy-Item -Force -LiteralPath $Source -Destination $Destination
    }
    else {
        "Same (or newer): $destinationFile"
    }
}

function Register-Components {
    Param(
        [Parameter(Mandatory = $true)][string]$BinPath,
        [Parameter(Mandatory = $true)][string]$DataPath
    )

    Show-Header 'Registering components'

    # Launch agent to write config files
    # Based on postinst : register_reference_extensions

    $aduciotagent_path = $BinPath + '/AducIotAgent.exe'

    $adu_extensions_dir = "$DataPath/extensions"
    $adu_extensions_sources_dir = "$adu_extensions_dir/sources"

    #
    # contentDownloader
    #

    # curl content downlaoader not used on Windows.
    # /var/lib/adu/extensions/content_downloader/extension.json
    # $curl_content_downloader_file = 'curl_content_downloader.dll'
    # & $aduciotagent_path --register-extension "$adu_extensions_sources_dir/$curl_content_downloader_file" --extension-type contentDownloader --log-level 2

    # /var/lib/adu/extensions/content_downloader/extension.json
    $do_content_downloader_file = "$adu_extensions_sources_dir/deliveryoptimization_content_downloader.dll"
    & $aduciotagent_path --register-extension $do_content_downloader_file --extension-type contentDownloader --log-level 2
    if ($LASTEXITCODE -ne 0) {
        Show-Error "Registration of '$do_content_downloader_file' failed: $LASTEXITCODE"
    }

    #
    # updateContentHandler
    #

    # /var/lib/adu/extensions/update_content_handlers/microsoft_script_1/content_handler.json
    $microsoft_script_1_handler_file = "$adu_extensions_sources_dir/microsoft_script_1.dll"
    & $aduciotagent_path --register-extension $microsoft_script_1_handler_file --extension-type updateContentHandler --extension-id 'microsoft/script:1' --log-level 2
    if ($LASTEXITCODE -ne 0) {
        Show-Error "Registration of '$do_content_downloader_file' failed: $LASTEXITCODE"
    }

    # /var/lib/adu/extensions/update_content_handlers/microsoft_simulator_1/content_handler.json
    $microsoft_simulator_1_file = "$adu_extensions_sources_dir/microsoft_simulator_1.dll"
    & $aduciotagent_path --register-extension $microsoft_simulator_1_file --extension-type updateContentHandler --extension-id 'microsoft/simulator:1'
    if ($LASTEXITCODE -ne 0) {
        Show-Error "Registration of '$do_content_downloader_file' failed: $LASTEXITCODE"
    }

    # /var/lib/adu/extensions/update_content_handlers/microsoft_steps_1/content_handler.json
    $microsoft_steps_1_file = "$adu_extensions_sources_dir/microsoft_steps_1.dll"
    & $aduciotagent_path --register-extension $microsoft_steps_1_file --extension-type updateContentHandler --extension-id 'microsoft/steps:1' --log-level 2
    if ($LASTEXITCODE -ne 0) {
        Show-Error "Registration of '$do_content_downloader_file' failed: $LASTEXITCODE"
    }
    & $aduciotagent_path --register-extension $microsoft_steps_1_file --extension-type updateContentHandler --extension-id 'microsoft/update-manifest' --log-level 2
    if ($LASTEXITCODE -ne 0) {
        Show-Error "Registration of '$do_content_downloader_file' failed: $LASTEXITCODE"
    }
    & $aduciotagent_path --register-extension $microsoft_steps_1_file --extension-type updateContentHandler --extension-id 'microsoft/update-manifest:4' --log-level 2
    if ($LASTEXITCODE -ne 0) {
        Show-Error "Registration of '$do_content_downloader_file' failed: $LASTEXITCODE"
    }
    & $aduciotagent_path --register-extension $microsoft_steps_1_file --extension-type updateContentHandler --extension-id 'microsoft/update-manifest:5' --log-level 2
    if ($LASTEXITCODE -ne 0) {
        Show-Error "Registration of '$do_content_downloader_file' failed: $LASTEXITCODE"
    }

    # microsoft/swupdate:1 not used on Windows.
    # /var/lib/adu/extensions/update_content_handlers/microsoft_swupdate_1/content_handler.json
    # $microsoft_simulator_1_file = "$adu_extensions_sources_dir/microsoft_swupdate_1.dll"
    # & $aduciotagent_path --register-extension $microsoft_simulator_1_file --extension-type updateContentHandler --extension-id 'microsoft/swupdate:1'
    # if ($LASTEXITCODE -ne 0) {
    #     Show-Error "Registration of '$do_content_downloader_file' failed: $LASTEXITCODE"
    # }

    # /var/lib/adu/extensions/update_content_handlers/microsoft_wim_1/content_handler.json
    $microsoft_wim_1_handler_file = "$adu_extensions_sources_dir/microsoft_wim_1.dll"
    & $aduciotagent_path --register-extension $microsoft_wim_1_handler_file --extension-type updateContentHandler --extension-id 'microsoft/wim:1'  --log-level 2
    if ($LASTEXITCODE -ne 0) {
        Show-Error "Registration of '$do_content_downloader_file' failed: $LASTEXITCODE"
    }
    ''
}

function Create-Adu-Folders {
    [Parameter(Mandatory = $true)][string]$BinPath,
    [Parameter(Mandatory = $true)][string]$DataPath,
    [Parameter(Mandatory = $true)][string]$LibPath

    # Not needed -- test data
    # mkdir -Force /tmp/adu/testdata | Out-Null

    # Unused on Windows
    # mkdir -Force /usr/local/lib/systemd/system | Out-Null

    mkdir -Force $BinPath | Out-Null

    mkdir -Force $LibPath | Out-Null

    mkdir -Force "$DataPath" | Out-Null
    mkdir -Force "$DataPath/diagnosticsoperationids" | Out-Null
    mkdir -Force "$DataPath/downloads" | Out-Null
    mkdir -Force "$DataPath/extensions/content_downloader" | Out-Null
    mkdir -Force "$DataPath/extensions/sources" | Out-Null
    mkdir -Force "$DataPath/sdc" | Out-Null
}

function Install-Adu-Components {
    Param(
        [Parameter(Mandatory = $true)][string]$BuildBinPath,
        [Parameter(Mandatory = $true)][string]$BuildLibPath,
        [Parameter(Mandatory = $true)][string]$BinPath,
        [Parameter(Mandatory = $true)][string]$LibPath,
        [Parameter(Mandatory = $true)][string]$DataPath
    )

    Show-Header 'Installing ADU Agent'

    Create-Adu-Folders `
        -BinPath $BinPath `
        -DataPath $DataPath `
        -LibPath $LibPath

    # contoso_component_enumerator
    # curl_content_downloader
    # microsoft_apt_1
    # microsoft_delta_download_handler

    Invoke-CopyFile "$BuildBinPath/adu-shell.exe" $LibPath
    Invoke-CopyFile  "$BuildBinPath/AducIotAgent.exe" $BinPath

    # IMPORTANT: Windows builds require these DLLS as well. Any way to build these statically?
    $dependencies = 'getopt', 'pthreadVC3d', 'libcrypto-1_1-x64'
    $dependencies | ForEach-Object {
        Invoke-CopyFile "$BuildBinPath/$_.dll" $LibPath
        Invoke-CopyFile "$BuildBinPath/$_.dll" $BinPath
    }

    # 'microsoft_swupdate_1', 'microsoft_swupdate_2'
    $extensions = 'deliveryoptimization_content_downloader', 'microsoft_script_1', 'microsoft_simulator_1', 'microsoft_steps_1', 'microsoft_wim_1'
    $extensions | ForEach-Object {
        Invoke-CopyFile  "$BuildLibPath/$_.dll" "$DataPath/extensions/sources"
    }

    if ($Type -eq 'Debug') {
        $pthread_dll = 'pthreadVC3d.dll'
    }
    else {
        $pthread_dll = 'pthreadVC3.dll'
    }
    Invoke-CopyFile "$BuildBinPath/$pthread_dll" $LibPath
    Invoke-CopyFile "$BuildBinPath/$pthread_dll" $BinPath

    ''

    # Healthcheck expects this file to be read-only.
    Set-ItemProperty -LiteralPath "$LibPath/adu-shell.exe" -Name IsReadOnly -Value $true

    Register-Components -BinPath $BinPath -DataPath $DataPath
}

function Create-DataFiles {
    Param(
        [Parameter(Mandatory = $true)][string]$ConfPath
    )

    Show-Header 'Creating data files'

    "Data file path: $ConfPath"
    ''

    mkdir -Force $ConfPath | Out-Null

    #
    # $env:TEMP/du-simulator-data.json (SIMULATOR_DATA_FILE)
    #
    #
    #     @'
    # {
    #     "isInstalled": {
    #         "*": {
    #             "resultCode": 901,
    #             "extendedResultCode": 0,
    #             "resultDetails": "simulated isInstalled"
    #         }
    #     },
    #     "download": {
    #         "*": {
    #             "resultCode": 500,
    #             "extendedResultCode": 0,
    #             "resultDetails": "simulated download"
    #         }
    #     },
    #     "install": {
    #         "resultCode": 600,
    #         "extendedResultCode": 0,
    #         "resultDetails": "simulated install"
    #     },
    #     "apply": {
    #         "resultCode": 700,
    #         "extendedResultCode": 0,
    #         "resultDetails": "simulated apply"
    #     }
    # }
    # '@ | Out-File -Encoding ASCII "$env:TEMP/du-simulator-data.json"

    #
    # /etc/adu/du-diagnostics-config.json
    #

    @'
{
    "logComponents": [
        {
            "componentName": "DU",
            "logPath": "/var/log/adu/"
        }
    ],
    "maxKilobytesToUploadPerLogPath": 5
}
'@ | Out-File -Encoding ASCII "$ConfPath/du-diagnostics-config.json"

    #
    # /etc/adu/du-config.json
    #

    if (-not (Test-Path -LiteralPath "$ConfPath/du-config.json" -PathType Leaf)) {
        @'
{
    "schemaVersion": "1.1",
    "aduShellTrustedUsers": [
        "adu",
        "do"
    ],
    "iotHubProtocol": "mqtt",
    "compatPropertyNames": "manufacturer,model",
    "manufacturer": "manufacturer",
    "model": "model",
    "agents": [
        {
            "name": "aduagent",
            "runas": "adu",
            "connectionSource": {
                "connectionType": "string",
                "connectionData": "[NOT_SPECIFIED]"
            },
            "$description": "manufacturer, model will be matched against update manifest 'compability' attributes",
            "manufacturer": "[NOT_SPECIFIED]",
            "model": "[NOT_SPECIFIED]"
        }
    ]
}
'@ | Out-File -Encoding ASCII "$ConfPath/du-config.json"
    }
}

function New-TemporaryDirectory {
    $dir = New-Item -ItemType Directory -Path (Join-Path ([IO.Path]::GetTempPath()) ([Guid]::NewGuid()))
    $dir.FullName
}

#
#  _ __  __ _(_)_ _
# | '  \/ _` | | ' \
# |_|_|_\__,_|_|_||_|
#

$root_dir = git.exe rev-parse --show-toplevel
if (!$root_dir) {
    Show-Error 'Unable to determine repo root.'
    exit 1
}

# Build folders
if (!$BuildOutputPath) {
    $BuildOutputPath = "$root_dir/out"
}
$runtime_dir = "$BuildOutputPath/bin"
$library_dir = "$BuildOutputPath/lib"

# Local install folders
$ETC_FOLDER = '/etc'
$USR_FOLDER = '/usr'
$VAR_FOLDER = '/var'
$ADUC_CONF_FOLDER = "$ETC_FOLDER/adu"
$ADUC_AGENT_FOLDER = "$USR_FOLDER/bin"
$ADUSHELL_FOLDER = "$USR_FOLDER/lib/adu"
$ADUC_DATA_FOLDER = "$VAR_FOLDER/lib/adu"
$ADUC_EXTENSIONS_FOLDER = "$ADUC_DATA_FOLDER/extensions"

#
# Install
#

if (-not (Test-Path '/tmp/adu/testdata' -PathType Container)) {
    Show-Warning 'Do clean build to copy test data to /tmp/adu/testdata'
}

Install-Adu-Components `
    -BuildBinPath "$runtime_dir/$Type" `
    -BuildLibPath "$library_dir/$Type" `
    -BinPath $ADUC_AGENT_FOLDER `
    -LibPath $ADUSHELL_FOLDER `
    -DataPath $ADUC_DATA_FOLDER

Create-DataFiles -ConfPath $ADUC_CONF_FOLDER

if (Select-String -Pattern '[NOT_SPECIFIED]' -LiteralPath "$ADUC_CONF_FOLDER/du-config.json" -SimpleMatch) {
    Show-Warning "Need to edit connectionData, agents.manufacturer and/or agents.model in $ConfPath/du-config.json"
}

#
# Package
#

if ($Package) {
    # /tmp/adu/testdata has test files, but we're not packaging test collateral.

    # Need to copy everything to a temporary directory, as Compress-Archive
    # can't persist relative paths.

    $temp = New-TemporaryDirectory
    $paths = $ETC_FOLDER, $USR_FOLDER, $VAR_FOLDER
    $paths | ForEach-Object {
        Copy-Item -Path $_ -Destination $temp -Recurse
    }

    $archive = '{0}/du-{1}.zip' -f ([Environment]::GetFolderPath('Desktop')), (Get-Date -Format FileDateTime)

    Compress-Archive `
        -CompressionLevel Optimal `
        -DestinationPath $archive `
        -Update `
        -Path "$temp/*"

    "Archive: $archive"

    Remove-Item -Recurse -Force $temp
}
