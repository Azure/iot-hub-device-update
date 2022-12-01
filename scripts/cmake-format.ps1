function error {
    Param([Parameter(mandatory = $true, position = 0)][string]$message)
    Write-Host -ForegroundColor Red -NoNewline 'Error:'
    Write-Host " $message"
}

if (-not (Get-Command 'git.exe' -CommandType Application  -ErrorAction SilentlyContinue)) {
    error 'git is not installed.'
    exit 1
}

if (-not (Get-Command 'cmake-format.exe' -CommandType Application -ErrorAction SilentlyContinue)) {
    error 'cmake-format is not installed.'
    exit 1
}

$GITROOT = git.exe rev-parse --show-toplevel

if (-not (Test-Path $GITROOT)) {
    error 'Unable to determine git root.'
    exit 1
}

$cmake_format_failed_or_reformatted = $false

Push-Location $GITROOT

# Use --cached to only check the staged files before commit.
# diff-filter=d will exclude deleted files.
git.exe diff --diff-filter=d --relative --name-only --cached HEAD -- 'CMakeLists.txt' '*/CMakeLists.txt' '*.cmake' '*/*.cmake' ` | ForEach-Object {
    $FILE = $_
    # Unfortunately, cmake-format doesn't work when piping git cat-file blob
    # into it so that it processes only staged file contents, but we use the
    # --check argument so that a reformatting results in non-zero exit code.
    cmake-format.exe --check $FILE 2>&1 | Out-Null
    if ($LastExitCode -ne 0) {
        $cmake_format_failed_or_reformatted = $true

        # now, in-place format the file. Unfortunately, it will be in the
        # unstaged working set, so we notify the user farther down.
        cmake-format.exe -i $FILE
    }
}

Pop-Location

if ($cmake_format_failed_or_reformatted) {
    error 'cmake-format failed or had to reformat a file. Fix the issues and/or git add the reformatted file(s).'
    exit 1
}

exit 0
