<#
.SYNOPSIS
Locally install / Package the Device Update product.

.DESCRIPTION
Assumes the product has been built locally using build.ps1

.EXAMPLE
PS> .\scripts\du_install.ps1 -Type Debug -Install -Package
#>

Param(
    # Output directory. Default is {git_root}/out/{Type}
    [string]$BuildOutputPath,
    # Build type
    [ValidateSet('Release', 'RelWithDebInfo', 'Debug')]
    [string]$Type = 'Debug',
    [ValidateSet('x64', 'arm64')]
    [string]$Arch = 'x64',
    # Build package?
    [switch]$Package = $false,
    # Run registration of components on this device?
    [switch]$RegisterComponents = $false
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

    # Healthcheck expects this file to be read-only.
    'Marking adu-shell.exe as read-only.'
    Set-ItemProperty -LiteralPath "$LibPath/adu-shell.exe" -Name IsReadOnly -Value $true

    Invoke-CopyFile  "$BuildBinPath/AducIotAgent.exe" $BinPath

    # Determine Windows Kits version.
    $WindowsKitsVer = Get-ChildItem -Path 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows Kits\Installed Roots' -Name | Sort-Object -Descending
    switch ($WindowsKitsVer.Count) {
        0 {
            Show-Error 'No Windows Kits installed'
            exit 1
        }
        1 {

        }
        default {
            $WindowsKitsVer = $WindowsKitsVer[0]
            Show-Warning "Multiple Windows Kits installed. Using latest ($WindowsKitsVer)."
        }
    }

    if ($Type -eq 'Debug') {
        $BuildToolsDPath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC\14.36.32532\debug_nonredist\$Arch\Microsoft.VC143.DebugCRT"
        $WindowsKitsDPath = "${env:ProgramFiles(x86)}\Windows Kits\10\bin\$WindowsKitsVer\$Arch\ucrt"

        # Only needed if dynamically linking: getopt, pthreadVC3d, libcrypto-1_1-x64

        $dependencies = `
        (Join-Path $BuildBinPath 'getopt'), `
        (Join-Path $BuildBinPath "libcrypto-1_1-$Arch"), `
        (Join-Path $BuildBinPath 'pthreadVC3d'), `
        (Join-Path $BuildToolsDPath 'msvcp140d'), `
        (Join-Path $BuildToolsDPath 'vcruntime140d'), `
        (Join-Path $BuildToolsDPath 'vcruntime140_1d'), `
        (Join-Path $WindowsKitsDPath 'ucrtbased')
    }
    else {
        $BuildToolsPath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC\14.36.32532\$Arch\Microsoft.VC143.CRT"
        $WindowsKitsPath = "${env:ProgramFiles(x86)}\Windows Kits\10\Redist\$WindowsKitsVer\ucrt\DLLs\$Arch"

        $dependencies = `
        (Join-Path $BuildBinPath 'getopt'), `
        (Join-Path $BuildBinPath "libcrypto-1_1-$Arch"), `
        (Join-Path $BuildBinPath 'pthreadVC3'), `
        (Join-Path $BuildToolsPath 'msvcp140'), `
        (Join-Path $BuildToolsPath 'vcruntime140'), `
        (Join-Path $BuildToolsPath 'vcruntime140_1'), `
        (Join-Path $WindowsKitsPath 'ucrtbase')
    }
    $dependencies | ForEach-Object {
        Invoke-CopyFile "$_.dll" $LibPath
        Invoke-CopyFile "$_.dll" $BinPath
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

    if ($RegisterComponents) {
        Register-Components -BinPath $BinPath -DataPath $DataPath
    }
}

function Create-DataFiles {
    Param(
        [Parameter(Mandatory = $true)][string]$ConfPath
    )

    "Data file path: $ConfPath"
    ''

    mkdir -Force $ConfPath | Out-Null

    #
    # /etc/adu/du-diagnostics-config.json
    #

    $dest = Join-Path $ConfPath 'du-diagnostics-config.json'

    if (-not (Test-Path -LiteralPath $dest -PathType Leaf)) {
        "Creating $dest"
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
'@ | Out-File -Encoding ASCII $dest
    }

    #
    # /etc/adu/du-config.json
    #

    $dest = Join-Path $ConfPath 'du-config.json'
    if (-not (Test-Path -LiteralPath $dest -PathType Leaf)) {
        "Creating $dest"
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
'@ | Out-File -Encoding ASCII $dest
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
    $BuildOutputPath = "$root_dir/out/$Type"
}

$BuildBinPath = "$BuildOutputPath/bin/$Type"
$BuildLibPath = "$BuildOutputPath/lib/$Type"

# Local install folders
$ETC_FOLDER = '/etc'
$USR_FOLDER = '/usr'
$VAR_FOLDER = '/var'

$ADUC_CONF_FOLDER = "$ETC_FOLDER/adu"
$ADUC_AGENT_FOLDER = "$USR_FOLDER/bin"
$ADUSHELL_FOLDER = "$USR_FOLDER/lib/adu"
$ADUC_DATA_FOLDER = "$VAR_FOLDER/lib/adu"
$ADUC_DOWNLOAD_FOLDER = "$VAR_FOLDER/lib/adu/downloads"
$ADUC_EXTENSIONS_FOLDER = "$ADUC_DATA_FOLDER/extensions"

# Create template data files, if they don't exist already.
Create-DataFiles -ConfPath $ADUC_CONF_FOLDER

# Install binaries locally
Install-Adu-Components `
    -BuildBinPath $BuildBinPath `
    -BuildLibPath $BuildLibPath `
    -BinPath $ADUC_AGENT_FOLDER `
    -LibPath $ADUSHELL_FOLDER `
    -DataPath $ADUC_DATA_FOLDER

# Sanity check
@($ADUC_CONF_FOLDER, $ADUC_AGENT_FOLDER, $ADUSHELL_FOLDER, $ADUC_DATA_FOLDER, $ADUC_DOWNLOAD_FOLDER, $ADUC_EXTENSIONS_FOLDER) `
| ForEach-Object {
    if (-not (Test-Path -LiteralPath $_ -PathType Container)) {
        Show-Error "Path not found: $_"
        exit 1
    }
}


if (Select-String -Pattern '[NOT_SPECIFIED]' -LiteralPath "$ADUC_CONF_FOLDER/du-config.json" -SimpleMatch) {
    Show-Warning "Need to edit connectionData, agents.manufacturer and/or agents.model in $ADUC_CONF_FOLDER/du-config.json"
}

#
# Package
#

if ($Package) {
    Show-Header 'Packaging Agent'

    # Not including:
    # /tmp/adu/testdata has test files, but we're not packaging test collateral.
    # $ADUC_CONF_FOLDER is configuration, and might have secrets.

    # Need to copy everything to a temporary directory, as Compress-Archive
    # can't persist relative paths into the archive.

    'Preparing directory . . .'

    $temp = New-TemporaryDirectory

    $dest = Join-Path $temp $ADUC_AGENT_FOLDER
    "$ADUC_AGENT_FOLDER => $dest"
    Copy-Item -Recurse -Path $ADUC_AGENT_FOLDER -Destination $dest
    $dest = Join-Path $temp $ADUSHELL_FOLDER
    "$ADUSHELL_FOLDER => $dest"
    Copy-Item -Recurse -Path $ADUSHELL_FOLDER -Destination $dest
    $dest = Join-Path $temp $ADUC_EXTENSIONS_FOLDER
    "$ADUC_EXTENSIONS_FOLDER => $dest"
    Copy-Item -Recurse -Path $ADUC_EXTENSIONS_FOLDER -Destination $dest

    $dest = Join-Path $temp (Join-Path $ADUC_AGENT_FOLDER 'symbols')
    "Symbols => $dest"
    New-Item -ItemType Directory -Path $dest | Out-Null
    Get-ChildItem `
        -Path $BuildBinPath `
        -Filter '*.pdb' `
        -Exclude '*unit_test.pdb', '*unit_tests.pdb', '*_ut.pdb', '*_tests_helper.pdb' `
        -Recurse `
    | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $dest
    }

    # Add git info as '_git-info.txt' in symbols folder.
    "Date: $(Get-Date)", "Head: $(git.exe rev-parse --short HEAD)", "Release: $(git.exe rev-parse --abbrev-ref HEAD)" `
    | Out-File -Encoding ascii  (Join-Path $dest '_git-info.txt')

    $dest = Join-Path $temp $ADUC_CONF_FOLDER
    New-Item -ItemType Directory -Path $dest | Out-Null

    Create-DataFiles -ConfPath $dest

    $dest = Join-Path $temp $ADUC_DOWNLOAD_FOLDER
    New-Item -ItemType Directory -Path $dest | Out-Null

    # -Format FileDateTime includes milliseconds, which is overkill.
    $archive = Join-Path ([IO.Path]::GetTempPath()) ('du-{0}-{1}.zip' -f $Type, (Get-Date -Format 'yyyyMMddTHHmmss'))

    ''
    "Archive file: $archive"

    Compress-Archive `
        -CompressionLevel Optimal `
        -DestinationPath $archive `
        -Update `
        -Path "$temp/*"

    Remove-Item -Recurse -Force $temp
}
