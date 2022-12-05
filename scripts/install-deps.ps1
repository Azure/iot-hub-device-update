# TODO(JeffMill): Add commandline params

function Error {
    Param([Parameter(mandatory = $true, position = 0)][string]$message)
    Write-Host -ForegroundColor Red -NoNewline 'Error:'
    Write-Host " $message"
}

function Invoke-WebRequestNoProgress {
    Param(
        [Parameter(mandatory = $true)][string]$Uri,
        [Parameter(mandatory = $true)][string]$OutFile)

    $PrevProgressPreference = $ProgressPreference

    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $Uri -OutFile $OutFile
    $ProgressPreference = $PrevProgressPreference
}

function Install-WithWinGet {
    Param(
        [Parameter(mandatory = $true)][string]$PackageId,
        [Parameter(mandatory = $true)][string]$TestExecutable)

    if (-not (Test-Path -LiteralPath $TestExecutable -PathType Leaf -ErrorAction SilentlyContinue)) {
        Write-Host -ForegroundColor Yellow "Installing $PackageId"
        winget.exe install $PackageId
    }
    else {
        Write-Host -ForegroundColor Green "$PackageId already installed."
    }
}

#
# OS VERSION CHECK
#

if ($env:OS -ne 'Windows_NT') {
    Error 'This script is intended to be run on Windows.'
    exit 1
}

if ($PSVersionTable.BuildVersion -lt '10.0.18362.0') {
    Error 'Requires Windows 10 1903 or later'
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
    Write-Host -ForegroundColor Green "winget already installed."
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
}
else {
    Write-Host -ForegroundColor Green 'VS Build Tools already installed.'
}

#
# CMAKE
#

Install-WithWinGet -PackageId 'Kitware.CMake' -TestExecutable "$env:ProgramFiles/CMake/bin/cmake.exe"

#
# PYTHON3
#

Install-WithWinGet -PackageId 'Python.Python.3.11' -TestExecutable "$env:LocalAppData/Programs/Python/Python311/python.exe"

#
# CMAKE-FORMAT
#

if (-not (Get-Command -Name 'cmake-format.exe' -CommandType Application -ErrorAction SilentlyContinue)) {
    python.exe -m pip install --upgrade pip
    pip.exe install cmake-format
}
else {
    Write-Host -ForegroundColor Green "cmake-format already installed."
}

#
# CLANG-FORMAT
#

if (-not (Get-Command -Name 'clang-format.exe' -CommandType Application -ErrorAction SilentlyContinue)) {
    python.exe -m pip install --upgrade pip
    pip.exe install clang-format
}
else {
    Write-Host -ForegroundColor Green "clang-format already installed."
}

#
# DOXYGEN, GRAPHVIZ (for --build_documentation)
#

Install-WithWinGet -PackageId 'DimitriVanHeesch.Doxygen' -TestExecutable "$env:ProgramFiles/doxygen/bin/doxygen.exe"

Install-WithWinGet -PackageId 'Graphviz.Graphviz' -TestExecutable "$env:ProgramFiles/Graphviz/bin/dot.exe"

#
# CPPCHECK
#

Install-WithWinGet -PackageId 'Cppcheck.Cppcheck' -TestExecutable "$env:ProgramFiles\Cppcheck\cppcheck.exe"
