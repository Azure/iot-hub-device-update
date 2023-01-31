if (-not (Get-Command 'git.exe' -CommandType Application  -ErrorAction SilentlyContinue)) {
    error 'git is not installed.'
    exit 1
}

$cmake_format_bin = 'cmake-format.exe'
if (-not (Get-Command $cmake_format_bin -CommandType Application -ErrorAction SilentlyContinue)) {
    $cmake_python_scripts_bin = "$env:LocalAppData\Packages\PythonSoftwareFoundation.Python.3.10_qbz5n2kfra8p0\LocalCache\local-packages\Python310\Scripts\cmake-format.exe"
    if (Test-Path $cmake_python_scripts_bin -PathType Leaf) {
        Write-Warning "Using non-PATH $cmake_format_bin ($cmake_python_scripts_bin)"
        $cmake_format_bin = $cmake_python_scripts_bin
    }
    else {
        error "cmake-format is not installed, or not in PATH"
        exit 1
    }
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
$files = @(git.exe diff --diff-filter=d --relative --name-only --cached HEAD -- 'CMakeLists.txt' '*/CMakeLists.txt' '*.cmake' '*/*.cmake')
for ($i = 0; $i -lt $files.count; $i++) {
    $FILE = $files[$i]

    "Formatting [{0,2}/{1,2}] {2}" -f ($i + 1), $files.count, $FILE

    # Unfortunately, cmake-format doesn't work when piping git cat-file blob
    # into it so that it processes only staged file contents, but we use the
    # --check argument so that a reformatting results in non-zero exit code.
    & $cmake_format_bin --check $FILE 2>&1 | Out-Null
    if ($LastExitCode -ne 0) {
        $cmake_format_failed_or_reformatted = $true

        # now, in-place format the file. Unfortunately, it will be in the
        # unstaged working set, so we notify the user farther down.
        & $cmake_format_bin -i $FILE
    }
}

Pop-Location

if ($cmake_format_failed_or_reformatted) {
    Write-Warning 'cmake-format failed or had to reformat a file. Fix the issues and/or git add the reformatted file(s).'
    exit 1
}

exit 0
