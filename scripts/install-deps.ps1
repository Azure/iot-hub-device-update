# TODO(JeffMill): Add commandline params

function Error {
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

# Use the Windows Store stub, as winget doesn't add to path, and so "python" ends up running the store stub.
# Install-WithWinGet -PackageId 'Python.Python.3.11' -TestExecutable "$env:LocalAppData/Programs/Python/Python311/python.exe"

$python_exe = "$env:LocalAppData/Microsoft/WindowsApps/PythonSoftwareFoundation.Python.3.10_qbz5n2kfra8p0/python3.exe"
if (-not (Test-Path $python_exe -PathType Leaf)) {
    Start-Process -Wait -FilePath "$env:LocalAppData/Microsoft/WindowsApps/python3.exe"
    Write-Host -ForegroundColor Yellow 'Click Get when the store window appears. Waiting for Install to complete.'
    while (-not (Test-Path $python_exe -PathType Leaf)) {
        Sleep -Seconds 1
    }
}


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
# DOXYGEN, GRAPHVIZ (for --build_documentation)
#

Install-WithWinGet -PackageId 'DimitriVanHeesch.Doxygen' -TestExecutable "$env:ProgramFiles/doxygen/bin/doxygen.exe"

Install-WithWinGet -PackageId 'Graphviz.Graphviz' -TestExecutable "$env:ProgramFiles/Graphviz/bin/dot.exe"

#
# CPPCHECK
#

Install-WithWinGet -PackageId 'Cppcheck.Cppcheck' -TestExecutable "$env:ProgramFiles\Cppcheck\cppcheck.exe"

''

