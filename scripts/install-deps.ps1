<#
.SYNOPSIS
Installs dependencies required to build the Device Update product.

.DESCRIPTION
Will check if dependencies are already installed, so it's okay to run this script multiple times.
In addition, this script can uninstall the installed dependencies.

It's recommended to run this script elevated.

.EXAMPLE
PS> ./Scripts/Install-deps.ps1
#>
Param(
    # Uninstall all of the apps that were installed with this script.
    [switch]$Uninstall,
    # Don't prompt the user.
    [switch]$Unattended
)

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

function Show-Error {
    Param([Parameter(Mandatory = $true, position = 0)][string]$message)
    Write-Host -ForegroundColor Red -NoNewline 'Error:'
    Write-Host " $message"
}

function Invoke-WebRequestNoProgress {
    Param(
        [Parameter(Mandatory = $true)][string]$Uri,
        [Parameter(Mandatory = $true)][string]$OutFile)

    $PrevProgressPreference = $ProgressPreference

    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $Uri -OutFile $OutFile
    $ProgressPreference = $PrevProgressPreference
}

function Install-WithWinGet {
    Param(
        [Parameter(Mandatory = $true)][string]$PackageId,
        [Parameter(Mandatory = $true)][string]$TestExecutable)

    if (-not (Test-Path -LiteralPath $TestExecutable -PathType Leaf -ErrorAction SilentlyContinue)) {
        Write-Host -ForegroundColor Yellow "Installing $PackageId"
        winget.exe install $PackageId
    }
    else {
        Write-Host -ForegroundColor Cyan "$PackageId already installed."
    }
}

#
# OS VERSION CHECK
#

if ($env:OS -ne 'Windows_NT') {
    Show-Error 'This script is intended to be run on Windows.'
    exit 1
}

if ($PSVersionTable.BuildVersion -lt '10.0.18362.0') {
    Show-Error 'Requires Windows 10 1903 or later'
    exit 1
}

if (Get-ComputerInfo -Property WindowsEditionId -eq 'IoTEnterpriseS')
{
    # Needs:
    # Microsoft.VCLibs.x64.14.00.Desktop.appx
    # Microsoft.UI.Xaml.2.7.appx
    # Microsoft.DesktopAppInstaller_8wekyb3d8bbwe.msixbundle
    Show-Error 'You must first manually install WinGet before running this script.'
    exit 1
}

if ('S-1-5-32-544' -notin ([System.Security.Principal.WindowsIdentity]::GetCurrent()).groups) {
    Write-Warning 'It''s recommended that you run this script as an administrator.'
}

if ($Uninstall) {
    if (!$Unattended) {
        $decision = $Host.UI.PromptForChoice(
            'Uninstall dependencies?',
            'Are you sure?',
            @(
            (New-Object Management.Automation.Host.ChoiceDescription '&Yes', 'Perform clean build'),
            (New-Object Management.Automation.Host.ChoiceDescription '&No', 'Stop build')
            ),
            1)
        if ($decision -ne 0) {
            return
        }
    }

    Show-Header 'Uninstalling'

    Show-Bullet 'CPPCHECK'
    winget.exe uninstall --disable-interactivity 'Cppcheck.CppCheck'
    Show-Bullet 'GRAPHVIZ'
    winget.exe uninstall --disable-interactivity 'Graphviz.Graphviz'
    Show-Bullet 'DOXYGEN'
    winget.exe uninstall --disable-interactivity 'DimitriVanHeesch.Doxygen'
    if ((Get-Command -Name 'pip.exe' -CommandType Application -ErrorAction SilentlyContinue)) {
        Show-Bullet 'CLANG-FORMAT'
        pip.exe uninstall --no-input clang-format
        Show-Bullet 'CMAKE-FORMAT'
        pip.exe uninstall --no-input cmake-format
    }
    Show-Bullet 'PYTHON 3'
    winget.exe uninstall --disable-interactivity 'Python.Python.3.11'
    winget.exe uninstall --disable-interactivity '{0E6EEAC9-4913-4C2F-B7D2-761B27C35D7C}' # Python launcher
    Show-Bullet 'CMAKE'
    winget.exe uninstall --disable-interactivity 'Kitware.CMake'
    Show-Bullet 'VS BUILD TOOLS'
    winget.exe uninstall --disable-interactivity 'Microsoft.VisualStudio.2022.BuildTools'
    Show-Bullet 'GIT'
    winget.exe uninstall --disable-interactivity 'Git.Git'

    # Note: Leaving WinGet installed.
    return
}

#
# WINGET
#

if (-not (Get-Command 'winget.exe' -CommandType Application -ErrorAction SilentlyContinue)) {
    Write-Host -ForegroundColor Yellow 'Installing App Installer (winget)'
    Invoke-WebRequestNoProgress -Uri 'https://github.com/microsoft/winget-cli/releases/download/v1.3.2691/Microsoft.DesktopAppInstaller_8wekyb3d8bbwe.msixbundle' -OutFile "$env:TEMP/Microsoft.DesktopAppInstaller_8wekyb3d8bbwe.msixbundle"
    Add-AppxPackage -Path "$env:TEMP/Microsoft.DesktopAppInstaller_8wekyb3d8bbwe.msixbundle"
}
else {
    Write-Host -ForegroundColor Cyan "winget already installed."
}


#
# GIT
#

Install-WithWinGet -PackageId 'Git.Git' -TestExecutable "$env:ProgramFiles/Git/bin/git.exe"

#
# VS BUILD TOOLS
#

if (-not (Test-Path -LiteralPath "${Env:ProgramFiles(x86)}/Microsoft Visual Studio/2022/BuildTools" -PathType Container)) {
    # Note:
    # Build Tools includes CMake, but it won't be in the path if Developer Tools
    # command window isn't open, so we'll install separately.
    # In addition, the website CMake is newer.

    # Note:
    # It would be nice to use "winget install 'Microsoft.VisualStudio.2022.BuildTools'"
    # but that installs C# tools, not C++ tools.
    # See https://github.com/microsoft/winget-pkgs/issues/87944

    Write-Host -ForegroundColor Yellow 'Installing VS Build Tools'
    Invoke-WebRequest -Uri 'https://aka.ms/vs/17/release/vs_BuildTools.exe' -OutFile "$env:TEMP/vs_BuildTools.exe"

    & "$env:TEMP/vs_BuildTools.exe" --passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --remove Microsoft.VisualStudio.Component.VC.CMake.Project

    'Waiting for installation to complete...'
    $anim = '(----------)','(---------*)','(--------*-)','(-------*--)','(------*---)','(-----*----)','(----*-----)','(---*------)','(--*-------)','(-*--------)','(*---------)'
    $animIndex = 0
    # Not the last file installed, but close enough...
    while (-not (Test-Path -LiteralPath "${Env:ProgramFiles(x86)}/Microsoft Visual Studio/2022/BuildTools/Common7/Tools/vsdevcmd/ext/ConnectionManagerExe.bat" -PathType Leaf)) {
        Write-Host -ForegroundColor Cyan -BackgroundColor Black -NoNewline ("`r{0}" -f $anim[$animIndex])
        $animIndex = ($animIndex + 1) % $anim.Length
        Start-Sleep -Milliseconds 500
    }
    ''
}
else {
    Write-Host -ForegroundColor Cyan 'VS Build Tools already installed.'
}

#
# CMAKE
#

Install-WithWinGet -PackageId 'Kitware.CMake' -TestExecutable "$env:ProgramFiles/CMake/bin/cmake.exe"

#
# PYTHON 3
#

Install-WithWinget -PackageId 'Python.Python.3.11' -TestExecutable "$env:LocalAppData/Programs/Python/Python311/python.exe"

# Temporarily add python.exe and pip.exe to PATH.
# Needs to come first to avoid the Windows python stub.
$env:PATH = "$env:LocalAppData\Programs\Python\Python311;$env:LocalAppData\Programs\Python\Python311\Scripts;" + $env:PATH

#
# CMAKE-FORMAT
#

if (-not (Get-Command -Name 'cmake-format.exe' -CommandType Application -ErrorAction SilentlyContinue)) {
    python.exe -m pip install --upgrade pip
    pip.exe install cmake-format
}
else {
    Write-Host -ForegroundColor Cyan "cmake-format already installed."
}

#
# CLANG-FORMAT
#

if (-not (Get-Command -Name 'clang-format.exe' -CommandType Application -ErrorAction SilentlyContinue)) {
    python.exe -m pip install --upgrade pip
    pip.exe install clang-format
}
else {
    Write-Host -ForegroundColor Cyan "clang-format already installed."
}

#
# DOXYGEN (for --build_documentation)
#

Install-WithWinGet -PackageId 'DimitriVanHeesch.Doxygen' -TestExecutable "$env:ProgramFiles/doxygen/bin/doxygen.exe"

#
# GRAPHVIZ (for --build_documentation)
#

Install-WithWinGet -PackageId 'Graphviz.Graphviz' -TestExecutable "$env:ProgramFiles/Graphviz/bin/dot.exe"

#
# CPPCHECK
#

Install-WithWinGet -PackageId 'Cppcheck.Cppcheck' -TestExecutable "$env:ProgramFiles\Cppcheck\cppcheck.exe"

''

# https://github.com/microsoft/terminal/issues/1125
'------------------------------------------------------------------------------'
'                     YOU MUST NOW COMPLETELY CLOSE AND REOPEN'
'                   WINDOWS TERMINAL FOR CHANGES TO TAKE EFFECT.'
'------------------------------------------------------------------------------'
if (!$Unattended) {
    while ($true) { }
}
