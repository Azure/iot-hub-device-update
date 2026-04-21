# Root Key Package Validator Tool

A standalone CLI tool that downloads a root key package from a URL and runs it through the full root key validation flow used by the Device Update agent.

## What it does

1. **Downloads** the root key package JSON from the specified URL using either curl or DeliveryOptimization
2. **Reads** the downloaded JSON file
3. **Parses** the JSON into an `ADUC_RootKeyPackage` structure
4. **Validates** the package signatures against the hardcoded root keys compiled into the agent

## Building

This tool is built as part of the main project. From your build directory:

```bash
# Configure (curl download support is included regardless of this flag)
cmake ..

# Build just the tool
cmake --build . --target rootkey_validator
```

Both curl and DeliveryOptimization downloaders are always compiled into the tool, regardless of the `ADUC_ROOTKEY_PKG_DOWNLOAD_WITH_CURL` build flag. You can select which downloader to use at runtime.

## Usage

```bash
# Run with defaults (curl downloader, default URL)
./rootkey_validator

# Use DeliveryOptimization downloader instead
./rootkey_validator --downloader do

# Use curl downloader explicitly
./rootkey_validator --downloader curl

# Run with a custom URL
./rootkey_validator --url "http://example.com/rootkeypackage.json"

# Specify a custom working directory for downloads
./rootkey_validator --workdir /tmp/my_rootkey_test

# Combine options
./rootkey_validator --downloader curl --url "http://example.com/rootkeypackage.json" --workdir /tmp/test

# Show help
./rootkey_validator --help
```

## Options

| Option         | Default | Description |
|----------------|---------|-------------|
| `--url`        | `http://granite-iothub-aat-dui--granite-iothub-aat-du.b.nlu.dl.adu.microsoft.com/SouthCentralUS/rootkeypackages/rootkeypackage-2.json` | URL of the root key package to download |
| `--workdir`    | `/tmp/rootkey_validator` | Working directory for downloaded files |
| `--downloader` | `curl` | Download method: `curl` or `do` (DeliveryOptimization) |

### Downloader notes

- **curl** — Shells out to `/usr/bin/curl`. Works on any system with curl installed. This is the default.
- **do** (DeliveryOptimization) — Uses the DO SDK. Requires the `deliveryoptimization-agent` service to be running.

## Exit codes

| Code | Meaning |
|------|---------|
| 0    | Root key package downloaded and validated successfully |
| 1    | Failure (download, parse, or validation error) |

## Example output

```
=== Root Key Package Validator Tool ===

URL:        http://granite-iothub-aat-dui--granite-iothub-aat-du.b.nlu.dl.adu.microsoft.com/SouthCentralUS/rootkeypackages/rootkeypackage-2.json
WorkDir:    /tmp/rootkey_validator
Downloader: Curl

[Step 1/4] Downloading root key package using Curl...
[Step 1/4] SUCCESS: Downloaded to /tmp/rootkey_validator/rootkey-validator-tool/rootkeypackage-2.json

[Step 2/4] Reading and parsing JSON...
[Step 2/4] SUCCESS: Read 3437 bytes of JSON

[Step 3/4] Parsing root key package structure...
[Step 3/4] SUCCESS: Root key package parsed successfully

[Step 4/4] Validating root key package with hardcoded keys...
[Step 4/4] SUCCESS: Root key package is VALID

=== Final Result ===
ResultCode: 1 (0x00000001)
ERC:        0 (0x00000000)
Overall:    PASSED