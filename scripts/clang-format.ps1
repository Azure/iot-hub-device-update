if (-not (Get-Command 'git.exe' -CommandType Application  -ErrorAction SilentlyContinue)) {
    error 'git is not installed.'
    exit 1
}

if (-not (Get-Command 'clang-format.exe' -CommandType Application -ErrorAction SilentlyContinue)) {
    error 'cmake-format is not installed. Try pip.exe install clang-format'
    exit 1
}

$GITROOT = git.exe rev-parse --show-toplevel

if (-not (Test-Path $GITROOT)) {
    error 'Unable to determine git root.'
    exit 1
}

Push-Location $GITROOT

# diff-filter=d will exclude deleted files.
git.exe diff --diff-filter=d --relative --name-only HEAD -- "*.[CcHh]" "*.[CcHh][Pp][Pp]" | ForEach-Object {
    clang-format.exe --verbose --style=file -i "$_"
}

Pop-Location
