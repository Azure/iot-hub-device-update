<#
.SYNOPSIS
Build the Device Update product.

.DESCRIPTION
Does all of the work to build the device update product, and provides additional support,
such as static checks and building documentation.

.EXAMPLE
PS> .\scripts\build.ps1 -Clean -Type Debug -BuildUnitTests
#>
Param(
    # Deletes targets before building.
    [switch]$Clean,
    # Build type
    [ValidateSet('Release', 'RelWithDebInfo', 'Debug')]
    [string]$Type = 'Debug',
    # Cross-compilation target
    [ValidateSet('ARM64')]
    [string]$GeneratorPlatform,
    # Should documentation be built?
    [switch]$BuildDocumentation,
    # Should unit tests be built?
    [switch]$BuildUnitTests,
    # Output directory. Default is {git_root}/out/{Type}
    [string]$BuildOutputPath,
    # Logging library to use
    [string]$LogLib = 'zlog',
    # Log folder to use for DU logs
    # TODO(JeffMill): Change this when folder structure determined.
    [string]$LogDir = '/var/log/adu',
    # Non-Interactive?
    [switch]$NoPrompt
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

function Show-CMakeErrors {
    Param([string[]]$BuildOutput)

    $regex = 'CMake Error at (?<File>.+):(?<Line>\d+)\s+\((?<Description>.+)\)'
    $result = $BuildOutput  | ForEach-Object {
        if ($_ -match $regex) {
            $matches
        }
    }
    if ($result.Count -ne 0) {
        Write-Host -ForegroundColor Red 'CMake errors:'

        $result | ForEach-Object {
            Show-Bullet  ("{0} ({1}): {2}" -f $_.File, $_.Line, $_.Description)
        }

        ''
    }
}

function Show-CompilerErrors {
    Param([string]$RootDir, [string[]]$BuildOutput)

    # Parse compiler errors
    # e.g. C:\du\src\logging\zlog\src\zlog.c(13,10): fatal  error C1083: Cannot open include file: 'aducpal/unistd.h': No such file or directory [C:\du\out\src\logging\zlog\zlog.vcxproj]
    $regex = '(?<File>.+)\((?<Line>\d+),(?<Column>\d+)\):.+error (?<Code>C\d+):\s+(?<Description>.+)\s+\[(?<Project>.+)\]'
    $result = $BuildOutput  | ForEach-Object {
        if ($_ -match $regex) {
            $matches
        }
    }
    if ($result.Count -ne 0) {
        Write-Host -ForegroundColor Red 'Compiler errors:'

        $result | ForEach-Object {
            Show-Bullet  ("{0} ({1},{2}): {3} [{4}]" -f $_.File.SubString($RootDir.Length + 1), $_.Line, $_.Column, $_.Description, (Split-Path $_.Project -Leaf))
        }

        ''
    }
}

function Show-LinkerErrors {
    Param([string]$RootDir, [string[]]$BuildOutput)

    # Parse linker errors

    $regex = '.+ error (?<Code>LNK\d+):\s+(?<Description>.+)\s+\[(?<Project>.+)\]'
    $result = $BuildOutput  | ForEach-Object {
        if ($_ -match $regex) {
            $matches
        }
    }
    if ($result.Count -ne 0) {
        Write-Host -ForegroundColor Red 'Linker errors:'

        $result | ForEach-Object {
            $project = $_.Project.SubString($RootDir.Length + 1)
            Show-Bullet  ("{0}: {1}" -f $project, $_.Description)
        }
        ''
    }

}

function Show-DoxygenErrors {
    Param([string]$RootDir, [string[]] $BuildOutput)

    # Parse Doxygen errors

    $regex = '\s*(?<File>.+?):(?<Line>\d+?): warning: (?<Message>.+)'
    $result = $BuildOutput | ForEach-Object {
        if ($_ -match $regex) {
            $matches
        }
    }
    if ($result.Count -ne 0) {
        Write-Host -ForegroundColor Red 'Doxygen errors:'

        $result | ForEach-Object {
            $file = $_.File.SubString($RootDir.Length + 1)
            Show-Bullet  ("{0} ({1}): {2}" -f $file, $_.Line, $_.Message)
        }
        ''
    }
}

function Install-DeliveryOptimization {
    Param(
        [Parameter(Mandatory = $true)][string]$Type,
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Branch,
        [string]$GeneratorPlatform,
        [string]$Commit)

    Show-Header 'Building Delivery Optimization'

    "Branch: $Branch"
    "Folder: $Path"
    ''

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        mkdir $Path | Out-Null
    }
    Push-Location $Path

    # do_url=git@github.com:Microsoft/do-client.git
    $do_url = 'https://github.com/Microsoft/do-client.git'

    # Avoid "fatal: destination path '.' already exists and is not an empty directory."
    if (-not (Test-Path '.git' -PathType Container)) {
        git.exe clone --recursive --single-branch --branch $Branch --depth 1 $do_url .
    }

    if ($Commit) {
        git.exe checkout $Commit
    }

    $build_dir = 'cmake'

    # Note: bootstrap-windows.ps1 installs CMake and Python, but we already installed those,
    # so not calling that script.

    # Note: install-vcpkg-deps.ps1 uses vcpkg to install
    # gtest:x64-windows,boost-filesystem:x64-windows,boost-program-options:x64-windows
    # but we can't use "vcpkg install" as we're in vcpkg manifest mode.

    if (-not (Test-Path -LiteralPath $build_dir -PathType Container)) {
        mkdir $build_dir | Out-Null
    }

    # Bug 43044349: DO-client cmakefile not properly building correct type using cmake
    # -DCMAKE_BUILD_TYPE should ultimately be removed.
    $DO_CMAKE_OPTIONS = @(
        '-DDO_BUILD_TESTS:BOOL=OFF',
        '-DDO_INCLUDE_SDK=ON',
        "-DCMAKE_BUILD_TYPE=$Type"
    )

    if ($GeneratorPlatform) {
        $DO_CMAKE_OPTIONS += "-A$GeneratorPlatform"
    }

    'CMAKE_OPTIONS:'
    $DO_CMAKE_OPTIONS | ForEach-Object { Show-Bullet $_ }
    ''

    & $cmake_bin -S . -B $build_dir @DO_CMAKE_OPTIONS
    & $cmake_bin --build $build_dir --config $Type --parallel

    Pop-Location
    ''
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

if (!$BuildOutputPath) {
    if (!$GeneratorPlatform) {
        $BuildOutputPath = "$root_dir/out/$Type"
    }
    else {
        $BuildOutputPath = "$root_dir/out/$GeneratorPlatform-$Type"
    }

}

if ($BuildDocumentation) {
    if (-not (Get-Command 'doxygen.exe' -CommandType Application -ErrorAction SilentlyContinue)) {
        Show-Error 'Can''t build documentation - doxygen is not installed or not in PATH.'
        exit 1
    }

    if (-not (Get-Command 'dot.exe' -CommandType Application -ErrorAction SilentlyContinue)) {
        Show-Error 'Can''t build documentation - dot (graphviz) is not installed or not in PATH.'
        exit 1
    }
}

# Set default log dir if not specified.
$runtime_dir = "$BuildOutputPath/bin"
$library_dir = "$BuildOutputPath/lib"

$cmake_bin = 'cmake.exe'
$cmake_cmd = Get-Command -CommandType Application cmake.exe
$cmake_bin = $cmake_cmd.Path
Write-Verbose "Using CMake path: $cmake_bin"

$PlatformLayer = 'windows'

# Output banner
''
Show-Header 'Building ADU Agent'
Show-Bullet "Clean build: $Clean"
Show-Bullet "Documentation: $BuildDocumentation"
Show-Bullet "Platform layer: $PlatformLayer"
Show-Bullet "Build type: $Type"
Show-Bullet "Log directory: $LogDir"
Show-Bullet "Logging library: $LogLib"
Show-Bullet "Output directory: $BuildOutputPath"
Show-Bullet "Build unit tests: $BuildUnitTests"
Show-Bullet "CMake: $cmake_bin"
Show-Bullet ("CMake version: {0}" -f (& $cmake_bin --version | Select-String  '^cmake version (.*)$').Matches.Groups[1].Value)
''

# Store options for CMAKE in an array
$DU_CMAKE_OPTIONS = @(
    "-DADUC_BUILD_DOCUMENTATION:BOOL=$BuildDocumentation",
    "-DADUC_BUILD_UNIT_TESTS:BOOL=$BuildUnitTests",
    "-DADUC_LOG_FOLDER:STRING=$LogDir",
    "-DADUC_LOGGING_LIBRARY:STRING=$LogLib",
    "-DADUC_PLATFORM_LAYER:STRING=$PlatformLayer",
    "-DCMAKE_BUILD_TYPE:STRING=$Type",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON",
    # Note: Multi-configuration generators (Visual Studio, Xcode, Ninja Multi-Config) append a
    # per-configuration subdirectory to the specified directory unless a generator expression is used.
    "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:STRING=$library_dir",
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:STRING=$runtime_dir"
)

if (-not $Clean) {
    # -ErrorAction SilentlyContinue doesn't work on Select-String
    try {
        if (-not (Select-String 'CMAKE_PROJECT_NAME:' "$BuildOutputPath/CMakeCache.txt")) {
            $Clean = $true
        }
    }
    catch {
        $Clean = $true
    }

    if ($Clean) {
        Show-Warning 'CMake cache seems out of date - forcing clean build.'
        ''
        $Clean = $true
    }
}

if ($Clean) {
    if (-not $NoPrompt) {
        $decision = $Host.UI.PromptForChoice(
            'Clean Build',
            'Are you sure?',
            @(
                (New-Object Management.Automation.Host.ChoiceDescription '&Yes', 'Perform clean build'),
                (New-Object Management.Automation.Host.ChoiceDescription '&No', 'Stop build')
            ),
            1)
        if ($decision -ne 0) {
            exit 1
        }
        ''
    }

    Show-Header 'Cleaning repo'

    if (Test-Path $BuildOutputPath) {
        Show-Bullet $BuildOutputPath
        Remove-Item -Force  -Recurse -LiteralPath $BuildOutputPath
    }

    ''
}

mkdir -Path $BuildOutputPath -Force | Out-Null

if ($Clean) {
    Show-Header 'Generating Makefiles'

    # show every find_package call (vcpkg specific):
    # $DU_CMAKE_OPTIONS += '-DVCPKG_TRACE_FIND_PACKAGE:BOOL=ON'
    # Verbose output (very verbose, but useful!):
    # $DU_CMAKE_OPTIONS += '--trace-expand'
    # See cmake dependencies (very verbose):
    # $DU_CMAKE_OPTIONS += '--debug-output'
    # See compiler output at build time:
    # $DU_CMAKE_OPTIONS += '-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON'
    # See library search output:
    # $DU_CMAKE_OPTIONS += '-DCMAKE_EXE_LINKER_FLAGS=/VERBOSE:LIB'

    if ($GeneratorPlatform) {
        $DU_CMAKE_OPTIONS += "-A$GeneratorPlatform"
    }

    'CMAKE_OPTIONS:'
    $DU_CMAKE_OPTIONS | ForEach-Object { Show-Bullet $_ }
    ''

    & $cmake_bin -S "$root_dir" -B $BuildOutputPath @DU_CMAKE_OPTIONS
    $ret_val = $LASTEXITCODE

    if ($ret_val -ne 0) {
        Write-Error "ERROR: CMake failed (Error $ret_val)"
        ''

        Show-CMakeErrors -BuildOutput $cmake_output

        exit $ret_val
    }

    ''
}

#
# Delivery Optimization
#

# Reusing ".vcpkg-installed" folder ... why not?
$DoPath = "{0}/.vcpkg-installed/do-client" -f (git.exe rev-parse --show-toplevel)
if ($GeneratorPlatform) {
    $DoPath += "-$GeneratorPlatform"
}
Install-DeliveryOptimization -Type $Type -Path $DoPath -Branch 'v1.0.1' -GeneratorPlatform $GeneratorPlatform

Show-Header 'Building Product'

& $cmake_bin --build $BuildOutputPath --config $Type --parallel 2>&1 | Tee-Object -Variable build_output
$ret_val = $LASTEXITCODE
''

if ($BuildDocumentation) {
    Show-DoxygenErrors -RootDir $root_dir -BuildOutput $build_output
}

if ($ret_val -ne 0) {
    Write-Host -ForegroundColor Red "ERROR: Build failed (Error $ret_val)"
    ''

    Show-CMakeErrors -BuildOutput $build_output

    Show-CompilerErrors -RootDir $root_dir -BuildOutput $build_output

    Show-LinkerErrors -RootDir $root_dir -BuildOutput $build_output

    exit $ret_val
}

exit $ret_val
