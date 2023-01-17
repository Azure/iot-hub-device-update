# e.g.
# scripts\build.ps1 --build-unit-tests --type RelWithDebInfo --platform-layer simulator --log-dir '/tmp/aduc-logs'
# TODO(JeffMill): Change arg parsing to standard PowerShell Param()
#
# TODO(JeffMill): Scenario 2:--static-analysis clang-tidy,cppcheck --build-docs

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
    Param([string[]]$BuildOutput)

    # Parse compiler errors
    # e.g. C:\wiot-s1\src\logging\zlog\src\zlog.c(13,10): fatal  error C1083: Cannot open include file: 'aducpal/unistd.h': No such file or directory [C:\wiot-s1\out\src\logging\zlog\zlog.vcxproj]
    $regex = '(?<File>.+)\((?<Line>\d+),(?<Column>\d+)\):.+error (?<Code>C\d+):\s+(?<Description>.+)\s+\[(?<Project>.+)\]'
    $result = $BuildOutput  | ForEach-Object {
        if ($_ -match $regex) {
            $matches
        }
    }
    if ($result.Count -ne 0) {
        Write-Host -ForegroundColor Red 'Compiler errors:'

        $result | ForEach-Object {
            Show-Bullet  ("{0} ({1},{2}): {3} [{4}]" -f $_.File.SubString($root_dir.Length + 1), $_.Line, $_.Column, $_.Description, (Split-Path $_.Project -Leaf))
        }

        ''
    }
}

function Show-LinkerErrors {
    Param([string[]]$BuildOutput)

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
            $project = $_.Project.SubString($root_dir.Length + 1)
            Show-Bullet  ("{0}: {1}" -f $project, $_.Description)
        }
        ''
    }

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
$default_log_dir = '/var/log/adu'
$output_directory = "$root_dir/out"
$build_unittests = $false
$static_analysis_tools = @()
$log_lib = 'zlog'
$install_prefix = '/usr/local'

# TODO(JeffMill): unused variable
# $install_adu = $false

# $work_folder = '/tmp'

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

function Invoke-CopyFile {
    Param(
        [Parameter(mandatory = $true)][string]$Source,
        [Parameter(mandatory = $true)][string]$Destination)

    if (-not (Test-Path -LiteralPath $Source -PathType Leaf)) {
        throw 'Source is not a file or doesnt exist'
    }

    if (-not (Test-Path -LiteralPath $Destination -PathType Container)) {
        throw 'Destination is not a folder'
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
        "$Source => $Destination"
        # Force, in case destination is marked read-only
        Copy-Item -Force -LiteralPath $Source -Destination $Destination
    }
    else {
        "$destinationFile is same or newer"
    }
}


function Create-Adu-Folders {
    # TODO(JeffMill): [PAL] Temporary until paths are determined.

    mkdir -Force /etc/adu | Out-Null
    mkdir -Force /tmp/adu/testdata | Out-Null
    mkdir -Force /usr/bin | Out-Null
    mkdir -Force /usr/lib/adu | Out-Null
    mkdir -Force /usr/local/lib/adu | Out-Null
    mkdir -Force /usr/local/lib/systemd/system | Out-Null
    mkdir -Force /var/lib/adu | Out-Null
    mkdir -Force /var/lib/adu/diagnosticsoperationids | Out-Null
    mkdir -Force /var/lib/adu/downloads | Out-Null
    mkdir -Force /var/lib/adu/extensions/content_downloader | Out-Null
    mkdir -Force /var/lib/adu/extensions/sources | Out-Null
    mkdir -Force /var/lib/adu/sdc | Out-Null
}

function Register-Components {
    # TODO(JeffMill): [PAL] Do correct component registration when ready.

    Show-Header 'Registering components'

    # Launch agent to write config files
    # see postinst : register_reference_extensions

    $adu_bin_path = '/usr/bin/AducIotAgent.exe'

    $adu_data_dir = '/var/lib/adu'
    $adu_extensions_dir = "$adu_data_dir/extensions"
    $adu_extensions_sources_dir = "$adu_extensions_dir/sources"

    $adu_steps_handler_file = 'microsoft_steps_1.dll'
    $curl_downloader_file = 'curl_content_downloader.dll'
    $adu_simulator_handler_file = 'microsoft_simulator_1.dll'

    # Will create /var/lib/adu/extensions/content_downloader/extension.json
    & $adu_bin_path --register-extension "$adu_extensions_sources_dir/$curl_downloader_file" --extension-type contentDownloader --log-level 2

    # Will create /var/lib/adu/extensions/update_content_handlers/microsoft_swupdate_1/content_handler.json
    # TODO(JeffMill): Temporary registration as microsoft/swupdate:1
    & $adu_bin_path --register-extension "$adu_extensions_sources_dir/$adu_simulator_handler_file" --extension-type updateContentHandler --extension-id 'microsoft/swupdate:1'

    # Will create e.g./var/lib/adu/extensions/update_content_handlers/microsoft_steps_1/content_handler.json
    & $adu_bin_path --register-extension "$adu_extensions_sources_dir/$adu_steps_handler_file" --extension-type updateContentHandler --extension-id "microsoft/steps:1" --log-level 2
    & $adu_bin_path --register-extension "$adu_extensions_sources_dir/$adu_steps_handler_file" --extension-type updateContentHandler --extension-id "microsoft/update-manifest" --log-level 2
    & $adu_bin_path --register-extension "$adu_extensions_sources_dir/$adu_steps_handler_file" --extension-type updateContentHandler --extension-id "microsoft/update-manifest:4" --log-level 2
    & $adu_bin_path --register-extension "$adu_extensions_sources_dir/$adu_steps_handler_file" --extension-type updateContentHandler --extension-id "microsoft/update-manifest:5" --log-level 2
}
function Install-Adu-Components {
    # TODO(JeffMill): [PAL] Temporary until paths are determined.

    Show-Header 'Installing ADU Agent'

    Create-Adu-Folders

    # cmake --install should place binaries, but that's not working correctly.
    # TODO(JWelden): Task 42936258: --install-prefix not working properly

    # & $cmake_bin --install $output_directory --config $build_type --verbose
    # $ret_val = $LASTEXITCODE
    # if ($ret_val -ne 0) {
    #     Write-Error "ERROR: CMake failed (Error $ret_val)"
    #     exit $ret_val
    # }

    # Workaround: Manually copy files

    $bin_path = "$runtime_dir/$build_type"
    # Invoke-CopyFile   deliveryoptimization_content_downloader
    # Invoke-CopyFile   microsoft_apt_1
    # Invoke-CopyFile   microsoft_delta_download_handler
    # Invoke-CopyFile   microsoft_script_1
    # Invoke-CopyFile   microsoft_swupdate_1
    # Invoke-CopyFile   microsoft_swupdate_2
    Invoke-CopyFile   "$bin_path/adu-shell.exe" /usr/lib/adu
    Invoke-CopyFile   "$bin_path/AducIotAgent.exe" /usr/bin
    Invoke-CopyFile   "$bin_path/contoso_component_enumerator.dll" /var/lib/adu/extensions/sources
    Invoke-CopyFile   "$bin_path/curl_content_downloader.dll" /var/lib/adu/extensions/sources
    Invoke-CopyFile   "$bin_path/microsoft_simulator_1.dll" /var/lib/adu/extensions/sources
    Invoke-CopyFile   "$bin_path/microsoft_steps_1.dll" /var/lib/adu/extensions/sources

    # IMPORTANT: Windows builds require these DLLS as well!
    # TODO(JeffMill): Any way to build these statically?

    Invoke-CopyFile  "$bin_path/libcrypto-1_1-x64.dll" /usr/lib/adu
    Invoke-CopyFile  "$bin_path/libcrypto-1_1-x64.dll" /usr/bin

    if ($build_type -eq 'Debug') {
        $pthread_dll = 'pthreadVC3d.dll'
    }
    else {
        $pthread_dll = 'pthreadVC3.dll'
    }
    Invoke-CopyFile  "$bin_path/$pthread_dll" /usr/lib/adu
    Invoke-CopyFile  "$bin_path/$pthread_dll" /usr/bin

    ''

    # Healthcheck expects this file to be read-only.
    Set-ItemProperty -LiteralPath '/usr/lib/adu/adu-shell.exe' -Name IsReadOnly -Value $true

    #
    # $env:TEMP/du-simulator-data.json (SIMULATOR_DATA_FILE)
    #

    @'
{
    "isInstalled": {
        "*": {
            "resultCode": 901,
            "extendedResultCode": 0,
            "resultDetails": "simulated isInstalled"
        }
    },
    "download": {
        "*": {
            "resultCode": 500,
            "extendedResultCode": 0,
            "resultDetails": "simulated download"
        }
    },
    "install": {
        "resultCode": 600,
        "extendedResultCode": 0,
        "resultDetails": "simulated install"
    },
    "apply": {
        "resultCode": 700,
        "extendedResultCode": 0,
        "resultDetails": "simulated apply"
    }
}
'@ | Out-File -Encoding ASCII "$env:TEMP/du-simulator-data.json"

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
'@ | Out-File -Encoding ASCII '/etc/adu/du-diagnostics-config.json'

    #
    # /etc/adu/du-config.json
    #

    if (-not (Test-Path -LiteralPath '/etc/adu/du-config.json' -PathType Leaf)) {
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
            "manufacturer": "jeffmillmfr",
            "model": "jeffmillmdl"
        }
    ]
}
'@ | Out-File -Encoding ASCII '/etc/adu/du-config.json'
    }
    if (Select-String -Pattern '[NOT_SPECIFIED]' -LiteralPath '/etc/adu/du-config.json' -SimpleMatch) {
        Show-Warning 'Need to edit connectionData,agents.manufacturer,agents.model in /etc/adu/du-config.json'
        ''
        notepad.exe '\etc\adu\du-config.json'
    }
    if (-not (Test-Path '/tmp/adu/testdata' -PathType Container)) {
        ''
        Show-Warning 'Do clean build to copy test data to /tmp/adu/testdata'
    }

    Register-Components
}

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
                Show-Error '-t build type parameter is mandatory.'
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
                Show-Error '-o output directory parameter is mandatory.'
                exit 1
            }
            $output_directory = $args[$arg]
        }

        { $_ -in '-s', '--static-analysis' } {
            $arg++
            if (-not $args[$arg]) {
                Show-Error '-s static analysis tools parameter is mandatory.'
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
                Show-Error '-p platform layer parameter is mandatory.'
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
                Show-Error '--log-lib parameter is mandatory.'
                exit 1
            }
            $log_lib = $args[$arg]
        }

        { $_ -in '-l', '--log-dir' } {
            $arg++
            if (-not $args[$arg]) {
                Show-Error '-l log directory  parameter is mandatory.'
                exit 1
            }
            $adu_log_dir = $args[$arg]
        }

        '--install-prefix' {
            $arg++
            if (-not $args[$arg]) {
                Show-Error '--install-prefix parameter is mandatory.'
                exit 1
            }
            $install_prefix = $args[$arg]
        }

        '--install' {
            $install_adu = $true
        }

        # '--cmake-path' {
        #     $arg++
        #     if (-not $args[$arg]) {
        #         Show-Error '--cmake-path is mandatory.'
        #         exit 1
        #     }
        #     $cmake_dir_path = $args[$arg]
        # }

        { $_ -in '-h', '--help' } {
            print_help
            exit 0
        }

        default {
            Show-Error "Invalid argument: $($args[$arg])"
            exit 1
        }
    }
    $arg++
}

if ($build_documentation) {
    if (-not (Get-Command 'doxygen.exe' -CommandType Application -ErrorAction SilentlyContinue)) {
        Show-Error 'Can''t build documentation - doxygen is not installed or not in PATH.'
        exit 1
    }

    if (-not (Get-Command 'dot.exe' -CommandType Application -ErrorAction SilentlyContinue)) {
        Show-Error 'Can''t build documentation - dot (graphviz) is not installed or not in PATH.'
        exit 1
    }
}

# TODO(JeffMill): Add a 'windows' platform layer. simulator seems to be broken.
if ($platform_layer -ne 'linux') {
    Show-Error 'Only linux platform layer is supported currently!'
    exit 1
}

# Set default log dir if not specified.
if (-not $adu_log_dir) {
    if ($platform_layer -eq 'simulator') {
        Show-Warning "Forcing adu_log_dir to /tmp/aduc-logs"
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
Show-Warning "Forcing content_handlers to $content_handlers"

# Output banner
''
Show-Header 'Building ADU Agent'
Show-Bullet "Clean build: $build_clean"
Show-Bullet "Documentation: $build_documentation"
Show-Bullet "Platform layer: $platform_layer"
Show-Bullet "Trace target deps: $trace_target_deps"
Show-Bullet "Content handlers: $content_handlers"
Show-Bullet "Build type: $build_type"
Show-Bullet "Log directory: $adu_log_dir"
Show-Bullet "Logging library: $log_lib"
Show-Bullet "Output directory: $output_directory"
Show-Bullet "Build unit tests: $build_unittests"
Show-Bullet "Build packages: $build_packages"
Show-Bullet "CMake: $cmake_bin"
Show-Bullet ("CMake version: {0}" -f (& $cmake_bin --version | Select-String  '^cmake version (.*)$').Matches.Groups[1].Value)
# Show-Bullet "shellcheck: $shellcheck_bin"
# Show-Bullet "shellcheck version: $("$shellcheck_bin" --version | grep 'version:' | awk '{ print $2 }')"
if ($static_analysis_tools.Length -eq 0) {
    Show-Bullet "Static analysis: (none)"
}
else {
    Show-Bullet "Static analysis: $static_analysis_tools"
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
            # Part of VS Build Tools'
            $clang_tidy_path = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/x64/bin/clang-tidy.exe"
            if (-not (Test-Path -LiteralPath $clang_tidy_path -PathType Leaf)) {
                Show-Error 'Can''t run static analysis - clang-tidy is not installed or not in PATH.'
                exit 1
            }

            $CMAKE_OPTIONS += "-DCMAKE_C_CLANG_TIDY:STRING=$clang_tidy_path"
            $CMAKE_OPTIONS += "-DCMAKE_CXX_CLANG_TIDY:STRING=$clang_tidy_path"
        }

        'cppcheck' {
            # winget install 'Cppcheck.Cppcheck'
            $cppcheck_path = "$env:ProgramFiles\Cppcheck\cppcheck.exe"
            if (-not (Test-Path -LiteralPath $cppcheck_path -PathType Leaf)) {
                Show-Error 'Can''t run static analysis - cppcheck is not installed or not in PATH.'
                exit 1
            }

            $CMAKE_OPTIONS += "-DCMAKE_CXX_CPPCHECK:STRING=$cppcheck_path;--template='{file}:{line}: warning: ({severity}) {message}';--platform=unix64;--inline-suppr;--std=c++11;--enable=all;--suppress=unusedFunction;--suppress=missingIncludeSystem;--suppress=unmatchedSuppression"
            $CMAKE_OPTIONS += "-DCMAKE_C_CPPCHECK:STRING=$cppcheck_path;--template='{file}:{line}: warning: ({severity}) {message}';--platform=unix64;--inline-suppr;--std=c99;--enable=all;--suppress=unusedFunction;--suppress=missingIncludeSystem;--suppress=unmatchedSuppression"
        }

        'cpplint' {
            Show-Error 'cpplint NYI'
            exit 1
        }

        'iwyu' {
            Show-Error 'iwyu NYI'
            exit 1
        }

        'lwyu' {
            Show-Error 'lwyu NYI'
            exit 1
        }

        default {
            Show-Warning "Invalid static analysis tool '$_'. Ignoring."
        }
    }
}

if (-not $build_clean) {
    # -ErrorAction SilentlyContinue doesn't work on Select-String
    try {
        if (-not (Select-String 'CMAKE_PROJECT_NAME:' "$output_directory/CMakeCache.txt")) {
            $build_clean = $true
        }
    }
    catch {
        $build_clean = $true
    }

    if ($build_clean) {
        Show-Warning 'CMake cache seems out of date - forcing clean build.'
        ''
        $build_clean = $true
    }
}

if ($build_clean) {
    Show-Header 'Cleaning repo'

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
        Show-Bullet $output_directory
        Remove-Item -Recurse -LiteralPath $output_directory
    }
    # TODO(JeffMill): This shouldn't hard code /tmp/adu/testdata -- it should match
    # whatever cmake and tests have.  JW has a bug on this.
    if (Test-Path '/tmp/adu/testdata') {
        Show-Bullet '/tmp/adu/testdata'
        Remove-Item -Recurse -LiteralPath '/tmp/adu/testdata'
    }

    ''
}

mkdir -Path $output_directory -Force | Out-Null

# TODO(JeffMill): Only generate makefiles on clean?
# It seems that dependencies are being generated unnecessarily otherwise?
if ($build_clean) {
    Show-Header 'Generating Makefiles'

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

    "CMAKE_OPTIONS:`n  {0}`n" -f ($CMAKE_OPTIONS -join "`n  ")

    & $cmake_bin -S "$root_dir" -B $output_directory @CMAKE_OPTIONS 2>&1 | Tee-Object -Variable cmake_output
    # TODO(JeffMill): Scenario 2: Use Ninja
    # & $cmake_bin -S "$root_dir" -B $output_directory  -G Ninja @CMAKE_OPTIONS
    $ret_val = $LASTEXITCODE

    if ($ret_val -ne 0) {
        Write-Error "ERROR: CMake failed (Error $ret_val)"
        ''

        Show-CMakeErrors -BuildOutput $cmake_output

        exit $ret_val
    }

    ''
}

Show-Header 'Building Product'

# TODO(JeffMill): Scenario 2: Use Ninja

& $cmake_bin --build $output_directory --config $build_type --parallel 2>&1 | Tee-Object -Variable build_output
$ret_val = $LASTEXITCODE
''

if ($ret_val -ne 0) {
    Write-Host -ForegroundColor Red "ERROR: Build failed (Error $ret_val)"
    ''

    Show-CMakeErrors -BuildOutput $build_output

    Show-CompilerErrors -BuildOutput $build_output

    Show-LinkerErrors -BuildOutput $build_output

    exit $ret_val
}

# if ($ret_val -eq 0 -and $build_packages) {
#     cpack
#     $ret_val=$LASTEXITCODE
# }

if ($ret_val -eq 0 -and $install_adu) {
    Install-Adu-Components
}

exit $ret_val

