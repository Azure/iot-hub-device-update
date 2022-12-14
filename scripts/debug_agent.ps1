function Error {
    Param([Parameter(mandatory = $true, position = 0)][string]$message)
    Write-Host -ForegroundColor Red -NoNewline 'Error:'
    Write-Host " $message"
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

$aduciotagent = "$root_dir\out\bin\RelWithDebInfo\AducIotAgent.exe"

if (-not (Test-Path $aduciotagent -PathType Leaf)) {
    Error 'Cannot find agent binary'
    exit 1
}

try {
    $windbgx = Get-WinDbgX
}
catch {
    Error "$_"
    exit 1
}

"Launching $windbgx . . ."
cmd.exe /c start $windbgx -c "bp aduciotagent!main; g" $aduciotagent

exit 0
