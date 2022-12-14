# e.g.
# scripts\build.ps1 --build-unit-tests --type RelWithDebInfo --platform-layer simulator --log-dir '/tmp/aduc-logs'
# TODO(JeffMill): Change arg parsing to standard PowerShell Param()
#
# TODO(JeffMill): Scenario 2:--static-analysis clang-tidy,cppcheck --build-docs

function Warn {
    Param([Parameter(mandatory = $true, position = 0)][string]$message)
    Write-Host -ForegroundColor Yellow -NoNewline 'Warning:'
    Write-Host " $message"
}

function Error {
    Param([Parameter(mandatory = $true, position = 0)][string]$message)
    Write-Host -ForegroundColor Red -NoNewline 'Error:'
    Write-Host " $message"
}

function Header {
    Param([Parameter(mandatory = $true, position = 0)][string]$message)
    $sep = ":{0}:" -f (' ' * ($message.Length + 2))
    Write-Host -ForegroundColor DarkYellow -BackgroundColor DarkBlue $sep
    Write-Host -ForegroundColor White -BackgroundColor DarkBlue ("  {0}  " -f $message)
    Write-Host -ForegroundColor DarkYellow -BackgroundColor DarkBlue $sep
    ''
}

function Bullet {
    Param([Parameter(mandatory = $true, position = 0)][string]$message)
    Write-Host -ForegroundColor Blue -NoNewline '*'
    Write-Host " $message"
}

$root_dir = git.exe rev-parse --show-toplevel

# TODO(JeffMill): Unused variable
# $script_dir = "$root_dir/scripts"

$build_clean = $false
$build_documentation = $false
$build_packages = $false
$platform_layer = 'linux'
$trace_target_deps = $false
$content_handlers = 'microsoft/swupdate,microsoft/apt,microsoft/simulator'
$build_type = 'Debug'
$adu_log_dir = ''
# $default_log_dir = '/var/log/adu'
# TODO(JeffMill): [PAL] Using this path for now due to ADUC bug assuming parent paths exist - cmakelists generates /tmp/adu
$default_log_dir='/tmp/adu/log'
$output_directory = "$root_dir/out"
$build_unittests = $false
$static_analysis_tools = @()
$log_lib = 'zlog'
$install_prefix = '/usr/local'

# TODO(JeffMill): unused variable
# $install_adu = $false

$work_folder = '/tmp'

# TODO(JeffMill): unused variable
# $cmake_dir_path = "$work_folder/deviceupdate-cmake"

function print_help {
    @'
Usage: build.ps1 [options...]
-c, --clean                           Does a clean build.
-t, --type <build_type>               The type of build to produce. Passed to CMAKE_BUILD_TYPE. Default is Debug.
                                      Options: Release Debug RelWithDebInfo MinSizeRel
-d, --build-docs                      Builds the documentation.
-u, --build-unit-tests                Builds unit tests.
--build-packages                      Builds and packages the client in various package formats e.g debian.
-o, --out-dir <out_dir>               Sets the build output directory. Default is out.
-s, --static-analysis <tools...>      Runs static analysis as part of the build.
                                      Tools is a comma delimited list of static analysis tools to run at build time.
                                      Tools: clang-tidy cppcheck cpplint iwyu lwyu (or all)

-p, --platform-layer <layer>          Specify the platform layer to build/use. Default is linux.
                                      Option: linux

--trace-target-deps                   Traces dependencies of CMake targets debug info.

--log-lib <log_lib>                   Specify the logging library to build/use. Default is zlog.
                                      Options: zlog xlog

-l, --log-dir <log_dir>               Specify the directory where the ADU Agent will write logs.
                                      Only valid for logging libraries that support file logging.

--install-prefix <prefix>             Install prefix to pass to CMake.

--install                             Install the following ADU components.
                                          From source: deviceupdate-agent.service & adu-swupdate.sh.
                                          From build output directory: AducIotAgent & adu-shell.

--cmake-path                          Override the cmake path such that CMake binary is at <cmake-path>/bin/cmake

-h, --help                            Show this help message.
'@
}

# TODO(JeffMill): Unreferenced
function CopyFile-ExitIfFailed {
    Param(
        [Parameter(mandatory = $true)][string]$Path,
        [Parameter(mandatory = $true)][string]$Destination)

    Header "Copying $Path to $Destination"
    if (-not (Copy-Item -LiteralPath $Path -Destination $Destination -ErrorAction SilentlyContinue)) {
        Error "Failed to copy $Path to $Destination (exit code: $($Error[0].CategoryInfo))"
        exit 1
    }
}

# TODO(JeffMill): Unreferenced
# function Install-Adu-Components {
# }

# TODO(JeffMill): Unreferenced
# function Determine-Distro {
# }

# For compatibility with SH script, emulate longopts
$arg = 0
while ($arg -lt $args.Count) {
    switch ($args[$arg]) {
        { $_ -in '-c', '--clean' } {
            $build_clean = $true
        }

        { $_ -in '-t', '--type' } {
            $arg++
            if (-not $args[$arg]) {
                Error '-t build type parameter is mandatory.'
                exit 1
            }
            $build_type = $args[$arg]
        }

        { $_ -in '-d', '--build-docs' } {
            $build_documentation = $true
        }

        { $_ -in '-u', '--build-unit-tests' } {
            $build_unittests = $true
        }

        '--build-packages' {
            $build_packages = $true
        }

        { $_ -in '-o', '--out-dir' } {
            $arg++
            if (-not $args[$arg]) {
                Error '-o output directory parameter is mandatory.'
                exit 1
            }
            $output_directory = $args[$arg]
        }

        { $_ -in '-s', '--static-analysis' } {
            $arg++
            if (-not $args[$arg]) {
                Error '-s static analysis tools parameter is mandatory.'
                exit 1
            }
            if ($args[$arg] -eq 'all') {
                $static_analysis_tools = @('clang-tidy', 'cppcheck', 'cpplint', 'iwyu', 'lwyu')
            }
            else {
                $static_analysis_tools = $args[$arg] -split ','
            }
        }

        { $_ -in '-p', '--platform-layer' } {
            $arg++
            if (-not $args[$arg]) {
                Error '-p platform layer parameter is mandatory.'
                exit 1
            }
            $platform_layer = $args[$arg]
        }

        '--trace-target-deps' {
            $trace_target_deps = $true
        }

        '--log-lib' {
            $arg++
            if (-not $args[$arg]) {
                Error '--log-lib parameter is mandatory.'
                exit 1
            }
            $log_lib = $args[$arg]
        }

        { $_ -in '-l', '--log-dir' } {
            $arg++
            if (-not $args[$arg]) {
                Error '-l log directory  parameter is mandatory.'
                exit 1
            }
            $adu_log_dir = $args[$arg]
        }

        '--install-prefix' {
            $arg++
            if (-not $args[$arg]) {
                Error '--install-prefix parameter is mandatory.'
                exit 1
            }
            $install_prefix = $args[$arg]
        }

        '--install' {
            $is_admin = (New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
            if (-not $is_admin) {
                Error 'Admin required to install ADU components.'
                exit 1
            }
            $install_adu = $true
        }

        '--cmake-path' {
            $arg++
            if (-not $args[$arg]) {
                Error '--cmake-path is mandatory.'
                exit 1
            }
            $cmake_dir_path = $args[$arg]
        }

        { $_ -in '-h', '--help' } {
            print_help
            exit 0
        }

        default {
            Error "Invalid argument: $($args[$arg])"
            exit 1
        }
    }
    $arg++
}

if ($build_documentation) {
    if (-not (Get-Command 'doxygen.exe' -CommandType Application -ErrorAction SilentlyContinue)) {
        Error 'Can''t build documentation - doxygen is not installed or not in PATH.'
        exit 1
    }

    if (-not (Get-Command 'dot.exe' -CommandType Application -ErrorAction SilentlyContinue)) {
        Error 'Can''t build documentation - dot (graphviz) is not installed or not in PATH.'
        exit 1
    }
}

# TODO(JeffMill): Add a 'windows' platform layer. simulator seems to be broken.
if ($platform_layer -ne 'linux') {
    Error 'Only linux platform layer is supported currently!'
    exit 1
}

# Set default log dir if not specified.
if (-not $adu_log_dir) {
    if ($platform_layer -eq 'simulator') {
        Warn "Forcing adu_log_dir to /tmp/aduc-logs"
        $adu_log_dir = '/tmp/aduc-logs'
    }
    else {
        $adu_log_dir = $default_log_dir
    }
}

$runtime_dir = "$output_directory/bin"
$library_dir = "$output_directory/lib"
$cmake_bin = 'cmake.exe'
# $shellcheck_bin = "$work_folder/deviceupdate-shellcheck"

# TODO(JeffMill): Forcing content_handlers in Windows builds.  Bug on Nox for this.
$content_handlers = 'microsoft/simulator'

# Output banner
''
Header 'Building ADU Agent'
Bullet "Clean build: $build_clean"
Bullet "Documentation: $build_documentation"
Bullet "Platform layer: $platform_layer"
Bullet "Trace target deps: $trace_target_deps"
Bullet "Content handlers: $content_handlers"
Bullet "Build type: $build_type"
Bullet "Log directory: $adu_log_dir"
Bullet "Logging library: $log_lib"
Bullet "Output directory: $output_directory"
Bullet "Build unit tests: $build_unittests"
Bullet "Build packages: $build_packages"
Bullet "CMake: $cmake_bin"
Bullet ("CMake version: {0}" -f (& $cmake_bin --version | Select-String  '^cmake version (.*)$').Matches.Groups[1].Value)
# Bullet "shellcheck: $shellcheck_bin"
# Bullet "shellcheck version: $("$shellcheck_bin" --version | grep 'version:' | awk '{ print $2 }')"
if ($static_analysis_tools.Length -eq 0) {
    Bullet "Static analysis: (none)"
}
else {
    Bullet "Static analysis: $static_analysis_tools"
}
''

# Store options for CMAKE in an array
$CMAKE_OPTIONS = @(
    "-DADUC_BUILD_DOCUMENTATION:BOOL=$build_documentation",
    "-DADUC_BUILD_UNIT_TESTS:BOOL=$build_unittests",
    "-DADUC_BUILD_PACKAGES:BOOL=$build_packages",
    "-DADUC_CONTENT_HANDLERS:STRING=$content_handlers",
    "-DADUC_LOG_FOLDER:STRING=$adu_log_dir",
    "-DADUC_LOGGING_LIBRARY:STRING=$log_lib",
    "-DADUC_PLATFORM_LAYER:STRING=$platform_layer",
    "-DADUC_TRACE_TARGET_DEPS=$trace_target_deps",
    "-DCMAKE_BUILD_TYPE:STRING=$build_type",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON",
    "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:STRING=$library_dir",
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:STRING=$runtime_dir",
    "-DCMAKE_INSTALL_PREFIX=$install_prefix"
)

$static_analysis_tools | ForEach-Object {
    switch ($_) {
        'clang-tidy' {
            # winget install llvm.llvm :
            # C:\Program Files\LLVM\bin\clang-tidy.exe
            # VS Build Tools:
            # C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\Llvm\bin
            $clang_tidy_path = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/bin/clang-tidy.exe"
            if (-not (Test-Path -LiteralPath $clang_tidy_path -PathType Leaf)) {
                Error 'Can''t run static analysis - clang-tidy is not installed or not in PATH.'
                exit 1
            }

            $CMAKE_OPTIONS += "-DCMAKE_C_CLANG_TIDY=$clang_tidy_path"
            $CMAKE_OPTIONS += "-DCMAKE_CXX_CLANG_TIDY=$clang_tidy_path"
        }

        'cppcheck' {
            # http://cppcheck.sourceforge.net/ (sudo apt install cppcheck)

            $cppcheck_path = "$env:ProgramFiles\Cppcheck\cppcheck.exe"
            if (-not (Test-Path -LiteralPath $cppcheck_path -PathType Leaf)) {
                Error 'Can''t run static analysis - cppcheck is not installed or not in PATH.'
                exit 1
            }

            # TODO(JeffMill): Fix these args for Windows
            $CMAKE_OPTIONS += "-DCMAKE_CXX_CPPCHECK=$cppcheck_path;--template='{file}:{line}: warning: ({severity}) {message}';--platform=unix64;--inline-suppr;--std=c++11;--enable=all;--suppress=unusedFunction;--suppress=missingIncludeSystem;--suppress=unmatchedSuppression;-I/usr/include;-I/usr/include/openssl"
            $CMAKE_OPTIONS += "-DCMAKE_C_CPPCHECK=$cppcheck_path;--template='{file}:{line}: warning: ({severity}) {message}';--platform=unix64;--inline-suppr;--std=c99;--enable=all;--suppress=unusedFunction;--suppress=missingIncludeSystem;--suppress=unmatchedSuppression;-I/usr/include;-I/usr/include/openssl"
        }

        'cpplint' {
            Error 'cpplint NYI'
            exit 1
        }

        'iwyu' {
            Error 'iwyu NYI'
            exit 1
        }

        'lwyu' {
            Error 'lwyu NYI'
            exit 1
        }

        default {
            Warn "Invalid static analysis tool '$_'. Ignoring."
        }
    }
}

if (-not (Test-Path '.\out\CMakeCache.txt' -PathType Leaf)) {
    Warn 'CMakeCache.txt not found - forcing clean build.'
    ''
    $build_clean = $true
}

if ($build_clean) {
    Header 'Cleaning repo'

    # TODO(JeffMill): Temporary prompting as I sometimes unintentionally specify "--clean". Sigh.
    $decision = $Host.UI.PromptForChoice(
        'Clean Build',
        'Are you sure?',
        @(
            (New-Object Management.Automation.Host.ChoiceDescription '&Yes', 'Perform clean build'),
            (New-Object Management.Automation.Host.ChoiceDescription '&No', 'Stop build')
        ),
        1)
    ''
    if ($decision -ne 0) {
        exit 1
    }


    if (Test-Path $output_directory) {
        Bullet $output_directory
        Remove-Item -Recurse -LiteralPath $output_directory
    }
    # TODO(JeffMill): This shouldn't hard code /tmp/adu/testdata -- it should match
    # whatever cmake and tests have.  JW has a bug on this.
    if (Test-Path '/tmp/adu/testdata') {
        Bullet '/tmp/adu/testdata'
        Remove-Item -Recurse -LiteralPath '/tmp/adu/testdata'
    }

    ''
}

mkdir -Path $output_directory -Force | Out-Null

# TODO(JeffMill): Only generate makefiles on clean?
# It seems that dependencies are being generated unnecessarily otherwise?
if ($build_clean) {
    Header 'Generating Makefiles'

    # Troubleshooting CMake dependencies
    # show every find_package call (vcpkg specific):
    # $CMAKE_OPTIONS += '-DVCPKG_TRACE_FIND_PACKAGE:BOOL=ON'
    # Verbose output (very verbose, but useful!):
    # $CMAKE_OPTIONS += '--trace-expand'
    # See cmake dependencies (very verbose):
    # $CMAKE_OPTIONS += '--debug-output'
    # See compiler output at build time:
    # $CMAKE_OPTIONS += '-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON'
    # See library search output:
    # $CMAKE_OPTIONS += '-DCMAKE_EXE_LINKER_FLAGS=/VERBOSE:LIB'

    & $cmake_bin -S "$root_dir" -B $output_directory @CMAKE_OPTIONS 2>&1 | Tee-Object -Variable cmake_output
    # TODO(JeffMill): Scenario 2: Use Ninja
    # & $cmake_bin -S "$root_dir" -B $output_directory  -G Ninja @CMAKE_OPTIONS
    $ret_val = $LASTEXITCODE

    if ($ret_val -ne 0) {
        error "ERROR: CMake failed (Error $ret_val)"
        ''

        # Parse CMake errors

        $regex = 'CMake Error at (?<File>.+):(?<Line>\d+) \((?<Description>.+)\)'
        $result = $cmake_output -split "`n" | ForEach-Object {
            if ($_ -match $regex) {
                $matches
            }
        }
        if ($result.Count -ne 0) {
            Write-Host -ForegroundColor Red 'CMake errors:'

            $result | ForEach-Object {
                Bullet  ("{0} ({1}): {2}" -f $_.File, $_.Line, $_.Description)
            }

            ''
        }

        exit $ret_val
    }

    ''
}

Header 'Building Product'

# TODO(JeffMill): Scenario 2: Use Ninja

& $cmake_bin --build $output_directory --config $build_type 2>&1 | Tee-Object -Variable build_output
$ret_val = $LASTEXITCODE
''

if ($ret_val -ne 0) {
    Write-Host -ForegroundColor Red "ERROR: Build failed (Error $ret_val)"
    ''

    # Parse CMake errors

    $regex = 'CMake Error at (?<File>.+):(?<Line>\d+) \((?<Description>.+)\)'
    $result = $build_output -split "`n" | ForEach-Object {
        if ($_ -match $regex) {
            $matches
        }
    }
    if ($result.Count -ne 0) {
        Write-Host -ForegroundColor Red 'CMake errors:'

        $result | ForEach-Object {
            Bullet  ("{0} ({1}): {2}..." -f $_.File, $_.Line, $_.Description)
        }

        ''
    }

    # Parse compiler errors

    $regex = '(?<File>.+)\((?<Line>\d+),(?<Column>\d+)\): error (?<Code>C\d+): (?<Description>.+) \[(?<Project>.+)\]'
    $result = $build_output -split "`n" | ForEach-Object {
        if ($_ -match $regex) {
            $matches
        }
    }
    if ($result.Count -ne 0) {
        Write-Host -ForegroundColor Red 'Compiler errors:'

        $result | ForEach-Object {
            Bullet  ("{0} ({1},{2}): {3} [{4}]" -f $_.File.SubString($root_dir.Length + 1), $_.Line, $_.Column, $_.Description, (Split-Path $_.Project -Leaf))
        }

        ''
    }

    # Parse linker errors

    $regex = '.+ error (?<Code>LNK\d+): (?<Description>.+) \[(?<Project>.+)\]'
    $result = $build_output -split "`n" | ForEach-Object {
        if ($_ -match $regex) {
            $matches
        }
    }
    if ($result.Count -ne 0) {
        Write-Host -ForegroundColor Red 'Linker errors:'

        $result | ForEach-Object {
            $project = $_.Project.SubString($root_dir.Length + 1)
            Bullet  ("{0}: {1}" -f $project, $_.Description)
        }
        ''
    }

    exit $ret_val
}

# if ($ret_val -eq 0 -and $build_packages) {
#     cpack
#     $ret_val=$LASTEXITCODE
# }

# if ($ret_val -eq 0 -and $install_adu) {
#     install_adu_components
# }

exit $ret_val

