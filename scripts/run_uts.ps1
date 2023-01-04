param(
    [switch]$ShowCTestOutput = $false,
    [switch]$RerunFailed = $false
)

$root_dir = git.exe rev-parse --show-toplevel
Push-Location "$root_dir/out"

if ($RerunFailed) {
    if ($ShowCTestOutput) {
        ctest.exe --rerun-failed --verbose | Tee-Object -Variable out
    }
    else {
        $out = ctest.exe --rerun-failed --verbose
    }
}
else {
    if ($ShowCTestOutput) {
        ctest.exe --verbose | Tee-Object -Variable out
    }
    else {
        $out = ctest.exe --verbose
    }
}

$lines = @()
$test_name = $null
$failure_count = 0

# Multiline regex matching would be nicer, but this works.
$out | ForEach-Object {
    $line = $_

    if ($line -match '^test\s+\d{1,5}') {
        # e.g.
        # test 107

        $lines = @()
        $test_name = $null
    }
    elseif ($line -match 'Test command: (.*)') {
        $test_name = $matches[1]
    }
    elseif ($line -match '\d{1,5}\/\d{1,5} Test\s+#\d{1,5}: (.*)\s*(\.+)\s*(.*?)\s*\d\.\d{2} sec') {
        # e.g.
        # 32/116 Test  #32: Bad http status retry info ...................................................***Exception: SegFault  0.02 sec
        # 84/116 Test  #84: Get update file by name - mixed case .........................................***Failed    0.02 sec
        # 86/116 Test  #86: Set workflow result ..........................................................   Passed    0.02 sec
        # 87/116 Test  #87: Get update manifest version - 1.0 ............................................   Passed    0.03 sec

        if ($matches[3].SubString(0, 3) -eq '***') {
            $failure_count++

            '~' * 79
            $test_name
            'Result: {0}' -f $matches[3].SubString(3)
            ''
            $lines
            '~' * 79
            ''
        }
    }
    else {
        $lines += $line.Trim()
    }
}

'{0} failed tests' -f $failure_count

Pop-Location

