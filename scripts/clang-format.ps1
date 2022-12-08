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
$files = @(git.exe diff --diff-filter=d --relative --name-only HEAD -- '*.[CcHh]' '*.[CcHh][Pp][Pp]')
for ($i = 0; $i -lt $files.count; $i++) {
    $FILE = $files[$i]

    "Formatting [{0,2}/{1,2}] {2}" -f ($i + 1), $files.count, $FILE

    clang-format.exe --style=file -i "$FILE"
}

Pop-Location
