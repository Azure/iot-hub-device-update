Param(
    # Filename to debug.
    [Parameter(Position = 0)][string]$FileName = 'AducIotAgent',
    # Set to build type, e.g. Debug, Release.
    [string]$Type = 'Debug',
    # Set to true if debugging a Catch2 Unit Test.
    [switch]$DebugCatchUT = $false,
    # Set to true if process launch without debugging is desired.
    [switch]$NoDebugger = $false,
    # Additional arguments to pass to command line.
    [string[]] $ArgumentList
)

# Example ArgumentList for AducIotAgent:
# '--register-extension', '/var/lib/adu/extensions/sources/microsoft_steps_1.dll', '--extension-type', 'updateContentHandler', '--extension-id', 'microsoft/steps:1'
# "--health-check"

# Example ArgumentList for adu-shell:
# '--update-type', 'common', '--update-action', 'reboot'



function Show-Error {
    Param([Parameter(mandatory = $true, position = 0)][string]$Message)
    Write-Host -ForegroundColor Red -NoNewline 'Error:'
    Write-Host " $Message"
}

function Show-Bullet {
    Param([Parameter(mandatory = $true, position = 0)][string]$Message)
    Write-Host -ForegroundColor Blue -NoNewline '*'
    Write-Host " $Message"
}

function Get-WinDbgX {
    Param(
        [String] $AppxPackage = 'Microsoft.WinDbg',
        [String] $Executable = 'DbgX.Shell.exe'
    )

    $pkg = Get-AppxPackage -Name $AppxPackage
    if (-not $pkg) {
        throw 'Unknown AppX Package: ' + $AppxPackage
    }
    $xml = [xml](Get-Content (Join-Path $pkg.InstallLocation 'AppxManifest.xml'))

    $pfn = $pkg.PackageFamilyName

    $appid = ($xml.Package.Applications.Application | Where-Object Executable -eq $Executable).id
    if (-not $appid) {
        throw 'Unknown Executable: ' + $Executable
    }

    "shell:appsFolder\$pfn!$appid"
}

$root_dir = git.exe rev-parse --show-toplevel
$bin_dir = "$root_dir\out\bin\$Type"
$app = Join-Path $bin_dir "$FileName.exe"

if (-not (Test-Path $app -PathType Leaf)) {
    Show-Error "Cannot find $app"

    if (-not (Test-Path $bin_dir -PathType Container)) {
        Show-Error "Folder $bin_dir doesn't exist. Either rebuild or specify -Type"
    }
    else {
        ''
        'The following apps were found:'
        (Get-ChildItem -LiteralPath $bin_dir -Filter '*.exe').BaseName | Sort-Object | ForEach-Object { Show-Bullet $_ }
    }
    exit 1
}

if ($NoDebugger) {
    "Launching $app . . ."
    & $app
}
else {

    try {
        $windbgx = Get-WinDbgX
    }
    catch {
        Show-Error "$_"
        'To install WinDbg use: winget install --accept-package-agreements ''WinDbg Preview'''
        exit 1
    }

    $module = $FileName -replace '-','_'

    if ($DebugCatchUT) {
        $InitialCommands = @( `
                # e.g. https_proxy_utils_unit_tests!`anonymous namespace'::C_A_T_C_H_T_E_S_T_0::test
                '-c', "bm $module!*::C_A_T_C_H_T_E_S_T_*::test", `
                # e.g. permission_utils_unit_test!C_A_T_C_H_T_E_S_T_0
                '-c', "bm $module!C_A_T_C_H_T_E_S_T_*", `
                # e.g. https_proxy_utils_unit_tests!TestCaseFixture::TestCaseFixture
                '-c', "bm $module!TestCaseFixture::TestCaseFixture", `
                '-c', "bm $module!TestCaseFixture::~TestCaseFixture", `
                # Start test
                '-c', 'g' `
        )

        $arguments = @("--success", "--break")
    }
    else {
        $InitialCommands = @( `
                '-c', "bp $module!main", `
                '-c', 'g' `
        )

        $arguments = @( `
            # Log level is DEBUG (very verbose) -- useful for debugging.
            '--log-level', '0'
        )
    }

    'Debugging: {0} {1} {2}' -f $app, ($arguments -join ' '), ($ArgumentList -join ' ')
    cmd.exe /c start $windbgx @InitialCommands $app @arguments @ArgumentList
}

exit 0
