Param(
    [Parameter(Position = 0)][string]$FileName = 'AducIotAgent',
    [string]$Type = 'RelWithDebInfo',
    # e.g. "bp aduciotagent!main; g"
    [string]$InitialCommand = 'g',
    [switch]$NoDebugger = $false,
    [String[]] $Arguments = $null
)

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

    ''
    'The following apps were found:'
    (Get-ChildItem -LiteralPath $bin_dir -Filter '*.exe').BaseName | Sort-Object | ForEach-Object { Show-Bullet $_ }
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

    "Debugging $app . . ."
    cmd.exe /c start $windbgx -c $InitialCommand $app @Arguments
}

exit 0
