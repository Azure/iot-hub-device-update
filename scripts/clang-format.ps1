function Error {
    Param([Parameter(mandatory = $true, position = 0)][string]$message)
    Write-Host -ForegroundColor Red -NoNewline 'Error:'
    Write-Host " $message"
}


if (-not (Get-Command 'git.exe' -CommandType Application  -ErrorAction SilentlyContinue)) {
    Error 'git is not installed.'
    exit 1
}

$clang_format_bin = 'clang-format.exe'
if (-not (Get-Command $clang_format_bin -CommandType Application -ErrorAction SilentlyContinue)) {
    $clang_python_scripts_bin = "$env:LocalAppData\Packages\PythonSoftwareFoundation.Python.3.10_qbz5n2kfra8p0\LocalCache\local-packages\Python310\Scripts\clang-format.exe"
    if (Test-Path $clang_python_scripts_bin -PathType Leaf) {
        Write-Warning "Using non-PATH $clang_format_bin ($clang_python_scripts_bin)"
        $clang_format_bin = $clang_python_scripts_bin
    }
    else {
        error "cmake-format is not installed, or not in PATH"
        exit 1
    }
}

$GITROOT = git.exe rev-parse --show-toplevel

if (-not (Test-Path $GITROOT)) {
    Error 'Unable to determine git root.'
    exit 1
}

Push-Location $GITROOT

# diff-filter=d will exclude deleted files.
$files = @(git.exe diff --diff-filter=d --relative --name-only HEAD -- '*.[CcHh]' '*.[CcHh][Pp][Pp]')
for ($i = 0; $i -lt $files.count; $i++) {
    $FILE = $files[$i]

    "Formatting [{0,2}/{1,2}] {2}" -f ($i + 1), $files.count, $FILE

    & $clang_format_bin --style=file -i "$FILE"
}

Pop-Location
