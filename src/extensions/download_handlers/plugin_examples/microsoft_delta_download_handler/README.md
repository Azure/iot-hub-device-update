# Microsoft Delta Download Handler

## Overview

The Microsoft Delta Download Handler is an extension for the Azure Device Update (ADU) agent that enables efficient delta-based firmware and software updates. Instead of downloading complete update files, this handler processes delta patches that contain only the differences between the current and target versions, significantly reducing bandwidth usage and update time.

Delta updates can reduce download sizes by 90-95% compared to full updates, making them ideal for devices with limited bandwidth, metered connections, or cellular networks. For example, a 800MB full update can be reduced to just 50-80MB with delta updates.

## Purpose

This download handler:
- Downloads and processes delta update files from Azure Device Update service
- Applies binary delta patches using `libadudiffapi` library (which implements bsdiff/bspatch algorithms)
- Validates downloaded content and applied patches using cryptographic hashes
- Integrates seamlessly with the ADU agent's update workflow
- Supports various compression (zstd, gzip) and delta algorithms through the `libadudiffapi` abstraction
- Manages source update cache for delta reconstruction
- Falls back to full download if delta application fails

## How Delta Updates Work

### High-Level Workflow

```
1. Device has Current Version (v1.0) installed
   └─> Cached in source update cache: /var/lib/adu/cache/v1.0.swu

2. Azure ADU Service has Target Version (v2.0)
   └─> Pre-generated delta file: v1.0-to-v2.0.diff (50MB)
   └─> Full update file available as fallback: v2.0.swu (800MB)

3. Download Handler Process:
   ├─> Check if source v1.0 exists in cache
   ├─> Download small delta file (50MB vs 800MB)
   ├─> Reconstruct target v2.0 from: source + delta
   ├─> Verify reconstructed file hash matches manifest
   └─> If success: skip full download | If fail: fallback to full download

4. Install Handler:
   └─> Install reconstructed v2.0.swu via SWUpdate

5. Post-Install:
   └─> Cache v2.0.swu for future delta updates (v2.0 → v3.0)
```

### Delta Reconstruction Process

```
┌─────────────────────────────────────────────────────────────┐
│              Delta Download & Reconstruction                │
└─────────────────────────────────────────────────────────────┘

Source Cache: /var/lib/adu/cache/
  └─> v1.0-recompressed.swu (800MB, zstd compressed ext4)

Download: /var/lib/adu/downloads/
  └─> v1.0-to-v2.0.diff (50MB, binary delta)

Reconstruction (via libadudiffapi):
  Input:  v1.0-recompressed.swu + v1.0-to-v2.0.diff
  Output: v2.0-recompressed.swu (800MB)

  // Conceptually equivalent to: bspatch old.swu new.swu delta.diff
  // Actual call: libadudiffapi->apply_diff(source, delta, target)
         │         │         └─> Downloaded diff file
         │         └─> Reconstructed target
         └─> Cached source

  Note: libadudiffapi is a higher-level library that wraps bsdiff/bspatch
  algorithms and handles SWU-specific archive processing and compression.

Verification:
  Compute SHA256 of v2.0-recompressed.swu
  Compare with hash from update manifest
  If match: Success, proceed to install
  If mismatch: Fail, fallback to full download
```

## Key Concepts

### Source Update Cache

The source update cache (`/var/lib/adu/cache/` or `/var/lib/adu/downloads/delta-cache/`) stores previously installed update files for use in future delta updates. When a full update is installed, the recompressed version is automatically cached for delta reconstruction.

**Cache Management:**
- Source updates must be recompressed with zstd compression
- Cache location: `/var/lib/adu/cache/` (default) or `/var/lib/adu/downloads/delta-cache/`
- Each cached file includes metadata for version matching
- Cache cleanup happens automatically based on available disk space

### Recompressed SWU Files

Delta updates require the source SWU file to be recompressed with zstd compression. This ensures:
- Consistent compression across source and target
- Efficient binary diff generation
- Reliable delta reconstruction

**Requirements:**
- SWUpdate must be built with `CONFIG_ZSTD=y`
- ext3/ext4 filesystems in SWU must use zstd compression
- Both source and target use identical compression settings

### Related Files in Update Manifest

The update manifest includes "relatedFiles" that specify delta files associated with different source versions:

```json
{
  "files": [
    {
      "filename": "v2.0.swu",
      "relatedFiles": [
        {
          "filename": "v1.0-to-v2.0.diff",
          "properties": {
            "microsoft.sourceFileHashAlgorithm": "sha256",
            "microsoft.sourceFileHash": "abc123...",
            "microsoft.sourceVersion": "1.0"
          }
        },
        {
          "filename": "v0.9-to-v2.0.diff",
          "properties": {
            "microsoft.sourceFileHash": "def456...",
            "microsoft.sourceVersion": "0.9"
          }
        }
      ]
    }
  ]
}
```

The handler iterates through relatedFiles and attempts delta reconstruction with each one until it finds a matching source in the cache.

## Architecture

The handler consists of:
- **Plugin** (`libmicrosoft_delta_download_handler.so`) - Shared library loaded by ADU agent
- **Handler Library** - Core implementation of delta download and processing logic
- **Utilities** - Helper functions for delta operations

## Dependencies

### Build-Time Dependencies

1. **Azure IoT Hub Device Update Delta Library** (`libadudiffapi`)
   - Source: https://github.com/Azure/iot-hub-device-update-delta
   - Provides delta creation, application, and validation functionality
   - Version: 3.0.0 or later
   - Header file (`adudiffapi.h`) must be in `/usr/include/` or `/usr/local/include/`

2. **Standard Build Tools**
   - CMake 3.23.2 or later
   - GCC/G++ 10 or later
   - Ninja build system
   - pkg-config

3. **ADU Agent Components**
   - `aduc::adu_types` - Core ADU types and interfaces
   - `aduc::c_utils` - Common C utilities
   - `aduc::logging` - Logging infrastructure
   - `aduc::source_update_cache` - Update caching functionality
   - `aduc::workflow_utils` - Workflow management utilities

4. **System Libraries**
   - libcurl (7.44 or later)
   - OpenSSL (3.0 or later)
   - libxml2

### Runtime Dependencies

1. **Delta Library** (`libadudiffapi.so`)
   - Must be installed in `/usr/lib/` or `/usr/local/lib/`

2. **Delta Library Dependencies** (typically bundled with `libadudiffapi.so`)
   - zlib - Compression support
   - zstd - Zstandard compression
   - bzip2 - BZip2 compression
   - OpenSSL - Cryptographic operations
   - jsoncpp - JSON parsing

3. **ADU Agent**
   - The handler is loaded as a plugin by the ADU agent at runtime

## Building the Handler

### Step 1: Install Dependencies

The recommended way to install all dependencies is using the provided installation script:

```bash
cd <adu-agent-root>
./scripts/install-deps.sh --install-all-deps --keep-source-code
```

This will:
- Install system packages
- Build and install Azure IoT SDK
- Build and install the delta library from source
- Install the delta library .deb package
- Copy the header file to `/usr/include/`
- Set up all other required dependencies

To install only the delta library dependency:

```bash
./scripts/install-deps.sh --install-delta --keep-source-code
```

### Step 2: Verify Delta Library Installation

Check that the delta library is properly installed:

```bash
# Check for the library
ls -l /usr/lib/libadudiffapi.so*
# or
ls -l /usr/local/lib/libadudiffapi.so*

# Check for the header
ls -l /usr/include/adudiffapi.h
# or
ls -l /usr/local/include/adudiffapi.h

# Verify the package (if installed via .deb)
dpkg -L ms-adu_diffs
```

### Step 3: Configure the Build

```bash
cd <adu-agent-root>
mkdir -p out
cd out

# Configure with delta handler enabled
cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DADUC_BUILD_DELTA_HANDLER=ON \
    -G Ninja \
    ..
```

Build options:
- `-DCMAKE_BUILD_TYPE=Debug` - Build with debug symbols (use `Release` for production)
- `-DADUC_BUILD_DELTA_HANDLER=ON` - Enable building the delta handler
- `-G Ninja` - Use Ninja build system (faster than Make)

### Step 4: Build the Handler

```bash
cd out

# Build everything
ninja

# Or build just the delta handler
ninja microsoft-delta-download-handler
```

The compiled plugin will be located at:
```
out/lib/libmicrosoft_delta_download_handler.so
```

### Step 5: Build the Package

```bash
ninja package
```

This creates two Debian packages:
1. **deviceupdate-agent-*.deb** - Main ADU agent package
2. **deviceupdate-agent-delta-*.deb** - Delta handler package

The delta handler package will be in:
```
out/deviceupdate-agent-delta-<version>-Linux.deb
```

## Troubleshooting Build Issues

### Issue: CMake Cannot Find Delta Library

**Symptoms:**
```
CMake Error: Could NOT find AzureIotHubDeviceUpdateDelta (missing:
  AzureIotHubDeviceUpdateDelta_INCLUDE_DIR)
```

**Solution:**
1. Verify delta library is installed:
   ```bash
   find /usr -name "libadudiffapi.so*" 2>/dev/null
   find /usr -name "adudiffapi.h" 2>/dev/null
   ```

2. If not found, install the delta library:
   ```bash
   ./scripts/install-deps.sh --install-delta
   ```

3. If built but not installed, manually install:
   ```bash
   # From .deb package
   sudo dpkg -i ~/.adu-tmp/iot-hub-device-update-delta/src/out/native/x64-linux/Debug/_packages/*.deb

   # Copy header file (package doesn't include it)
   sudo cp ~/.adu-tmp/iot-hub-device-update-delta/src/native/diffs/api/adudiffapi.h /usr/include/
   ```

4. Clear CMake cache and reconfigure:
   ```bash
   cd <adu-agent-root>/out
   rm -rf CMakeCache.txt CMakeFiles
   cmake -DADUC_BUILD_DELTA_HANDLER=ON -G Ninja ..
   ```

### Issue: Packaging Fails with "Cannot Find libmicrosoft_delta_download_handler.so"

**Symptoms:**
```
CMake Error at packages/cmake_install.cmake:46 (file):
  file INSTALL cannot find libmicrosoft_delta_download_handler.so
```

**Solution:**
1. Verify the handler was built:
   ```bash
   find <adu-agent-root>/out -name "libmicrosoft_delta_download_handler.so"
   ```

2. If not found, rebuild:
   ```bash
   cd <adu-agent-root>/out
   ninja microsoft-delta-download-handler
   ```

3. Check build logs for compilation errors:
   ```bash
   cd <adu-agent-root>/out
   ninja microsoft-delta-download-handler 2>&1 | tee build.log
   ```

### Issue: Delta Library Warning During Package Build

**Symptoms:**
```
CMake Warning: Delta library (libadudiffapi.so) not found in /usr/lib or /usr/local/lib
```

**Solution:**
The delta library must be in a system location for packaging. Install it properly:
```bash
# If you have the .deb package
sudo dpkg -i ~/.adu-tmp/iot-hub-device-update-delta/src/out/native/x64-linux/Debug/_packages/*.deb

# Update library cache
sudo ldconfig
```

### Issue: Linker Errors Related to Delta Library

**Symptoms:**
```
undefined reference to adudiff_*
```

**Solution:**
1. Ensure the delta library is installed system-wide
2. Check that ldconfig knows about it:
   ```bash
   sudo ldconfig
   ldconfig -p | grep adudiffapi
   ```

3. Verify the library exports the required symbols:
   ```bash
   nm -D /usr/lib/libadudiffapi.so | grep adudiff
   ```

## Installation

### Install Delta Handler Package

```bash
# Install the delta handler package
sudo dpkg -i deviceupdate-agent-delta-<version>-Linux.deb

# Verify installation
dpkg -L deviceupdate-agent-delta | grep -E '\.so$'
```

The handler will be installed to:
```
/var/lib/adu/extensions/sources/libmicrosoft_delta_download_handler.so
```

### Install Dependencies

The delta handler package depends on:
- `libadudiffapi.so` - Must be installed separately via `ms-adu_diffs` package
- System libraries (zlib, zstd, bzip2, openssl, jsoncpp) - Should be declared as package dependencies

```bash
# Install the delta library package first
sudo dpkg -i ms-adu_diffs_*.deb

# Then install the delta handler
sudo dpkg -i deviceupdate-agent-delta-*.deb
```

### Verify Installation

```bash
# Check handler plugin
ls -l /var/lib/adu/extensions/sources/libmicrosoft_delta_download_handler.so

# Check delta library
ls -l /usr/lib/libadudiffapi.so

# Verify dependencies
ldd /var/lib/adu/extensions/sources/libmicrosoft_delta_download_handler.so
```

All libraries should be found. If any show "not found", install the missing dependency.

## Testing the Handler

### Unit Tests

The handler includes unit tests that can be run during development. Tests must be enabled during the CMake configuration:

```bash
cd <adu-agent-root>
mkdir -p out
cd out

# Configure with delta handler and unit tests enabled
cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DADUC_BUILD_DELTA_HANDLER=ON \
    -DADUC_BUILD_UNIT_TESTS=ON \
    -G Ninja \
    ..

# Build everything including tests
ninja

# Run the delta handler tests
# Option 1: Run the test executable directly
./bin/microsoft_delta_download_handler_util_unit_tests

# Option 2: Run via ctest (Catch2 discovers individual test cases)
ctest --output-on-failure -R ".*"
# Or list all tests and filter manually
ctest -N | grep -i delta
```

**Note:** If using the `build.sh` script, use the `-u` flag to enable unit tests:
```bash
./scripts/build.sh -u --build-delta
```

**Verify tests were built:**
```bash
# Check if test executable exists
ls -l out/bin/microsoft_delta_download_handler_util_unit_tests

# Run the test executable directly to see all test cases
./out/bin/microsoft_delta_download_handler_util_unit_tests --list-tests
```

The test suite uses Catch2, which automatically discovers and registers individual test cases.

### Integration Testing

1. **Prepare Test Environment**
   ```bash
   # Ensure ADU agent is installed
   sudo dpkg -i deviceupdate-agent-*.deb

   # Install delta handler
   sudo dpkg -i deviceupdate-agent-delta-*.deb

   # Verify ADU agent configuration
   cat /etc/adu/du-config.json
   ```

2. **Configure ADU Agent for Delta Updates**

   Ensure your device is registered with Azure Device Update and configured to receive delta updates.

3. **Monitor Handler Loading**
   ```bash
   # Check ADU agent logs
   sudo journalctl -u deviceupdate-agent -f

   # Look for handler initialization messages
   grep "delta.*handler" /var/log/adu/adu.log
   ```

4. **Deploy a Delta Update**

   From Azure Portal or CLI:
   - Create a delta update package
   - Deploy it to your device or device group
   - Monitor the update process

5. **Verify Handler Execution**
   ```bash
   # Check agent logs for delta handler activity
   sudo journalctl -u deviceupdate-agent | grep -i delta

   # Check for downloaded delta files
   ls -l /var/lib/adu/downloads/

   # Verify update cache
   ls -l /var/lib/adu/cache/
   ```

### Manual Handler Testing

For development and debugging, you can test the handler in isolation:

1. **Check Handler Symbol Export**
   ```bash
   nm -D /var/lib/adu/extensions/sources/libmicrosoft_delta_download_handler.so | grep GetContentDownloaderUpdateActionFunc
   ```

2. **Verify Handler Dependencies**
   ```bash
   ldd /var/lib/adu/extensions/sources/libmicrosoft_delta_download_handler.so
   ```

3. **Test Delta Operations**

   Use the delta library command-line tools:
   ```bash
   # Create a test delta
   /usr/ms-adu_diffs/bin/bsdiff old_file new_file test.delta

   # Apply the delta
   /usr/ms-adu_diffs/bin/bspatch old_file patched_file test.delta

   # Verify result
   diff new_file patched_file
   ```

## Troubleshooting Runtime Issues

### Handler Not Loaded

**Symptoms:**
- ADU agent doesn't recognize delta updates
- No delta handler logs in agent output

**Diagnosis:**
```bash
# Check if handler file exists
ls -l /var/lib/adu/extensions/sources/libmicrosoft_delta_download_handler.so

# Check file permissions (should be readable by ADU agent user)
ls -l /var/lib/adu/extensions/sources/

# Verify ADU agent is looking in the right place
grep -i extension /etc/adu/du-config.json

# Check for loading errors
sudo journalctl -u deviceupdate-agent | grep -i "load.*handler\|extension"
```

**Solutions:**
1. Reinstall the delta handler package
2. Verify file permissions allow ADU agent to read the handler
3. Check ADU agent configuration for extension paths
4. Restart ADU agent: `sudo systemctl restart deviceupdate-agent`

### Issue: Delta Download Fails

**Symptoms:**
- Update fails during download phase
- Error messages about delta processing

**Diagnosis:**
```bash
# Check agent logs
sudo journalctl -u deviceupdate-agent -n 200

# Check download directory
ls -l /var/lib/adu/downloads/

# Check available disk space
df -h /var/lib/adu/

# Verify network connectivity to ADU service
curl -I https://du.azurefd.net
```

**Solutions:**
1. Ensure sufficient disk space for download and processing (at least 3x the rootfs size)
2. Check network connectivity and firewall rules
3. Verify ADU service credentials in configuration
4. Check delta file integrity on the service side
5. Review extended result codes in agent logs for specific failures

### Delta Application Fails

**Symptoms:**
- Download succeeds but patch application fails
- Validation errors after applying delta

**Diagnosis:**
```bash
# Check for core dumps
ls -l /var/crash/

# Check agent logs for specific error codes
sudo journalctl -u deviceupdate-agent | grep -i "error\|fail"

# Check source cache
ls -l /var/lib/adu/cache/
ls -l /var/lib/adu/downloads/delta-cache/

# Verify bspatch is available
which bspatch
bspatch --version

# Check reconstruction space
df -h /var/lib/adu/
```

**Solutions:**
1. Verify source file in cache matches expected hash
2. Check delta file wasn't corrupted during download
3. Ensure sufficient disk space for reconstruction (need space for: source + delta + target)
4. Verify delta library installation: `dpkg -l | grep ms-adu_diffs`
5. Check that source and target use same compression (zstd)
6. Verify SWUpdate was built with CONFIG_ZSTD=y

### Source Not Found in Cache

**Symptoms:**
- Handler reports "source update not found"
- Falls back to full download even though device has correct version

**Diagnosis:**
```bash
# Check cache directories
ls -lR /var/lib/adu/cache/
ls -lR /var/lib/adu/downloads/delta-cache/

# Check if source files exist but hash doesn't match
cat /var/lib/adu/cache/*/metadata.json

# Verify current installed version
cat /etc/adu-version
```

**Solutions:**
1. Ensure previous update properly cached the source:
   - Use `microsoft-delta-source-caching.sh` script handler
   - Or use swupdate handler v2 with proper caching configuration
2. Manually populate cache with recompressed source files
3. Redeploy the source version update to populate cache
4. Check that file hashes in cache match manifest expectations
5. Verify cache permissions allow ADU agent to read files

### Hash Mismatch After Reconstruction

**Symptoms:**
- Delta application completes but hash verification fails
- Error code indicates validation failure

**Diagnosis:**
```bash
# Check reconstructed file
ls -l /var/lib/adu/downloads/work-folder/

# Compute hash manually
sha256sum /var/lib/adu/downloads/work-folder/*.swu

# Compare with manifest hash
sudo journalctl -u deviceupdate-agent | grep -i "hash\|manifest"

# Check for compression mismatches
file /var/lib/adu/cache/*.swu
file /var/lib/adu/downloads/work-folder/*.swu
```

**Solutions:**
1. Verify source and delta files were generated correctly on build server
2. Ensure source file in cache is the recompressed version
3. Check that delta generation tool used matching source
4. Verify no corruption during download (check file sizes)
5. Regenerate delta files with correct parameters

### Performance Issues

**Symptoms:**
- Delta reconstruction takes very long time
- System becomes unresponsive during update

**Diagnosis:**
```bash
# Check memory usage
free -h
cat /proc/meminfo

# Check swap usage
swapon --show

# Monitor bspatch process
top -p $(pgrep bspatch)

# Check disk I/O
iostat -x 2 10
```

**Solutions:**
1. Ensure adequate RAM or swap space (recommend 2GB+ for large updates)
2. Use faster storage for /var/lib/adu/ (e.g., eMMC vs SD card)
3. Monitor and clean up old cached files to free space
4. Consider splitting large updates into smaller components
5. Adjust delta generation parameters for better performance

# Verify source files exist and are readable
ls -l /path/to/source/files

# Check delta library logs
dmesg | grep -i adu
```

**Solutions:**
1. Verify source file matches expected version/hash
2. Check delta file wasn't corrupted during download
3. Ensure sufficient disk space for temporary files
4. Verify delta library installation: `dpkg -l | grep ms-adu_diffs`

### Missing Delta Library Dependencies

**Symptoms:**
```
error while loading shared libraries: libadudiffapi.so: cannot open shared object file
```

**Diagnosis:**
```bash
# Check if library is installed
ls -l /usr/lib/libadudiffapi.so*

# Check if library is in cache
ldconfig -p | grep adudiffapi

# Check handler dependencies
ldd /var/lib/adu/extensions/sources/libmicrosoft_delta_download_handler.so
```

**Solutions:**
```bash
# Install delta library package
sudo dpkg -i ms-adu_diffs_*.deb

# Update library cache
sudo ldconfig

# Verify installation
ldconfig -p | grep adudiffapi

# Restart ADU agent
sudo systemctl restart deviceupdate-agent
```

## Output Artifacts

### Development Build
- Handler library: `out/lib/libmicrosoft_delta_download_handler.so`
- Static libraries: `out/lib/libmicrosoft_delta_download_handler_*.a`

### Package Build
- **deviceupdate-agent-delta-<version>-Linux.deb**
  - Handler plugin: `/var/lib/adu/extensions/sources/libmicrosoft_delta_download_handler.so`
  - Delta library: `/usr/lib/adu-delta/libadudiffapi.so` (bundled copy)
  - Control scripts: Pre/post install/remove scripts

## Configuration

The handler uses the ADU agent's standard extension mechanism. No additional configuration is typically required beyond:

1. **ADU Agent Configuration** (`/etc/adu/du-config.json`)
   - Must include extension loading configuration
   - Download handler extension path must be set

2. **Update Manifest**
   - Server-side configuration specifying delta update type
   - Handler selection criteria in update metadata

## End-to-End Integration Guide

### Prerequisites

1. **SWUpdate Configuration**
   - Build SWUpdate with zstd support: `CONFIG_ZSTD=y`
   - Verify with: `swupdate --help | grep -i zstd`

2. **ADU Agent Installation**
   - Install base agent: `deviceupdate-agent-*.deb`
   - Install delta handler: `deviceupdate-agent-delta-*.deb`
   - Install delta library: `ms-adu_diffs_*.deb`

3. **System Requirements**
   - Disk space: At least 3x rootfs size for reconstruction
   - RAM/Swap: 2GB+ recommended for large images
   - Filesystem: ext4 with journaling for update cache

### Step 1: Build and Generate Delta Files

**On Build Server:**

```bash
# Build base image (v1.0)
bitbake adu-base-image

# Build update image (v2.0) with recompression
bitbake adu-update-image-v2

# Generate delta files
bitbake adu-delta-image

# Output:
#   - v1.0.swu (base, 800MB)
#   - v1.0-recompressed.swu (zstd compressed)
#   - v2.0.swu (target, 800MB)
#   - v2.0-recompressed.swu (zstd compressed)
#   - v1.0-to-v2.0.diff (delta, 50MB)
```

**Delta Generation Recipe:**

Yocto recipe automatically:
1. Extracts ext4 filesystems from both SWU files
2. Generates binary diff using bsdiff
3. Creates recompressed SWU files with zstd compression
4. Verifies round-trip reconstruction
5. Packages files for distribution

### Step 2: Create Update Manifests

**Import Manifest for v1.0 (Full Update with Caching):**

```json
{
  "updateId": {
    "provider": "Contoso",
    "name": "RaspberryPi",
    "version": "1.0.0"
  },
  "instructions": {
    "steps": [
      {
        "handler": "microsoft/script:1",
        "files": ["v1.0.swu", "v1.0-recompressed.swu"],
        "handlerProperties": {
          "scriptFileName": "microsoft-delta-source-caching.sh",
          "installedCriteria": "1.0.0",
          "arguments": "--image-file v1.0.swu"
        }
      }
    ]
  },
  "files": {
    "v1.0.swu": {
      "filename": "v1.0.swu",
      "sizeInBytes": 838860800,
      "hashes": {
        "sha256": "abc123..."
      }
    },
    "v1.0-recompressed.swu": {
      "filename": "v1.0-recompressed.swu",
      "sizeInBytes": 838860800,
      "hashes": {
        "sha256": "def456..."
      }
    }
  }
}
```

**Import Manifest for v2.0 (Delta Update):**

```json
{
  "updateId": {
    "provider": "Contoso",
    "name": "RaspberryPi",
    "version": "2.0.0"
  },
  "instructions": {
    "steps": [
      {
        "handler": "microsoft/swupdate:2",
        "files": ["v2.0.swu"],
        "handlerProperties": {
          "swuFileName": "v2.0-recompressed.swu",
          "scriptFileName": "microsoft-delta-source-caching.sh",
          "installedCriteria": "2.0.0",
          "arguments": "--image-file v2.0-recompressed.swu"
        }
      }
    ]
  },
  "files": {
    "v2.0.swu": {
      "filename": "v2.0-recompressed.swu",
      "sizeInBytes": 838860800,
      "hashes": {
        "sha256": "ghi789..."
      },
      "downloadHandlerId": "microsoft/delta:1",
      "relatedFiles": [
        {
          "filename": "v1.0-to-v2.0.diff",
          "sizeInBytes": 52428800,
          "hashes": {
            "sha256": "jkl012..."
          },
          "properties": {
            "microsoft.sourceFileHashAlgorithm": "sha256",
            "microsoft.sourceFileHash": "def456...",
            "microsoft.sourceVersion": "1.0.0"
          }
        }
      ]
    },
    "v1.0-to-v2.0.diff": {
      "filename": "v1.0-to-v2.0.diff",
      "sizeInBytes": 52428800,
      "hashes": {
        "sha256": "jkl012..."
      }
    }
  }
}
```

### Step 3: Deploy Updates

**First Update (v0 → v1.0):**

```bash
# Device starts with v0 (factory image)
# Deploy v1.0 full update
az iot device-update deployments create \
  --account-name <account> \
  --instance-name <instance> \
  --deployment-id v1-deployment \
  --update-provider Contoso \
  --update-name RaspberryPi \
  --update-version 1.0.0

# What happens:
# 1. Download v1.0.swu (800MB)
# 2. Download v1.0-recompressed.swu (800MB)
# 3. Install v1.0.swu via SWUpdate
# 4. Cache v1.0-recompressed.swu to /var/lib/adu/downloads/delta-cache/
# 5. Reboot to new partition
```

**Second Update (v1.0 → v2.0 via Delta):**

```bash
# Deploy v2.0 delta update
az iot device-update deployments create \
  --account-name <account> \
  --instance-name <instance> \
  --deployment-id v2-deployment \
  --update-provider Contoso \
  --update-name RaspberryPi \
  --update-version 2.0.0

# What happens:
# 1. Delta handler checks cache for v1.0-recompressed.swu ✓
# 2. Download v1.0-to-v2.0.diff (50MB) - 93% bandwidth savings!
# 3. Reconstruct v2.0-recompressed.swu from cache + diff
# 4. Verify hash matches manifest
# 5. Skip downloading full v2.0.swu (saved 750MB!)
# 6. Install v2.0-recompressed.swu via SWUpdate
# 7. Cache v2.0-recompressed.swu for future deltas
# 8. Reboot to new partition
```

### Step 4: Monitor and Verify

**Check Delta Handler Activity:**

```bash
# Watch agent logs
sudo journalctl -u deviceupdate-agent -f | grep -i delta

# Check cache status
ls -lh /var/lib/adu/downloads/delta-cache/

# Verify current version
cat /etc/adu-version

# Check download statistics
sudo journalctl -u deviceupdate-agent | grep -i "download.*complete\|bytes"
```

**Expected Log Sequence for Delta Update:**

```
[INFO] Update manifest contains downloadHandlerId: microsoft/delta:1
[INFO] Loading download handler: libmicrosoft_delta_download_handler.so
[INFO] Delta handler: Processing update with 1 related files
[INFO] Delta handler: Checking cache for source version 1.0.0
[INFO] Delta handler: Source found in cache: /var/lib/adu/downloads/delta-cache/v1.0-recompressed.swu
[INFO] Delta handler: Downloading delta file: v1.0-to-v2.0.diff (50MB)
[INFO] Download complete: v1.0-to-v2.0.diff
[INFO] Delta handler: Reconstructing target from source + delta
[INFO] Executing: bspatch <source> <target> <delta>
[INFO] Reconstruction complete: v2.0-recompressed.swu
[INFO] Delta handler: Verifying reconstructed file hash
[INFO] Hash verification passed: sha256:ghi789...
[INFO] Delta handler: Returning SuccessSkipDownload (will not download full file)
[INFO] Install phase: Installing v2.0-recompressed.swu
```

### Step 5: Troubleshooting Integration

**Verify Delta Pipeline:**

```bash
# 1. Check handler is installed
ls -l /var/lib/adu/extensions/sources/libmicrosoft_delta_download_handler.so

# 2. Check delta library
ls -l /usr/lib/libadudiffapi.so
ldd /var/lib/adu/extensions/sources/libmicrosoft_delta_download_handler.so

# 3. Check SWUpdate has zstd support
swupdate --help | grep -i zstd

# 4. Verify cache directory exists and is writable
test -w /var/lib/adu/downloads/delta-cache/ && echo "OK" || echo "FAIL"

# 5. Check disk space
df -h /var/lib/adu/

# 6. Test bspatch manually
bspatch /var/lib/adu/downloads/delta-cache/v1.0-recompressed.swu \
        /tmp/test-output.swu \
        /var/lib/adu/downloads/v1.0-to-v2.0.diff

sha256sum /tmp/test-output.swu  # Should match manifest
```

## Observability and Logging

The delta download handler provides detailed logging with specific prefixes and patterns to help operators monitor delta update operations, diagnose issues, and measure bandwidth savings.

### Log Prefixes and Their Meanings

| Prefix | Purpose | When to Look For |
|--------|---------|------------------|
| `[DELTA]` | Delta-specific operations | All delta reconstruction events |
| `[TIMING]` | Performance metrics | Diagnosing slow operations |

### Success Indicators

**Successful Delta Reconstruction:**

Look for these log entries to confirm delta updates are working and saving bandwidth:

```
[DELTA] Source update found in cache at '/var/lib/adu/cache/...' - proceeding with delta reconstruction
[DELTA] Starting reconstruction: source='...', delta='...', target='...'
[DELTA] libadudiffapi apply succeeded - target file created at '/path/to/target'
[DELTA] Reconstruction SUCCESS - Downloaded 52428800 bytes (delta) instead of 838860800 bytes (full), saved 786432000 bytes (93%)
```

**Key Success Log Patterns:**
```bash
# Check for successful delta reconstructions with bandwidth savings
journalctl -u deviceupdate-agent | grep "\[DELTA\] Reconstruction SUCCESS"

# Example output:
# [DELTA] Reconstruction SUCCESS - Downloaded 50MB (delta) instead of 800MB (full), saved 750MB (93%)
```

**Successful Cache Operations:**

```
[TIMING] CacheSourceUpdate: Starting pre-reboot cache operation
[TIMING] CacheSourceUpdate: Cache operation completed in 1234 ms
[TIMING] CacheSourceUpdate: SUCCESS - Source update cached in 1234 ms before reboot
```

### Failure Indicators

**Source Cache Miss (Expected for first update):**

```
[DELTA] Source update not found in cache - cannot perform delta reconstruction
src update cache miss for Delta 0
```

This is expected when the device doesn't have a cached source file. The handler will either try the next relatedFile or fall back to full download.

**Delta Reconstruction Failed:**

```
[DELTA] libadudiffapi apply FAILED with error code: 123
diff apply - errcode 123: 'Error description from library'
[DELTA] Reconstruction FAILED for all 2 delta(s) - falling back to full download (838860800 bytes)
```

**Cache Operation Failed:**

```
[TIMING] CacheSourceUpdate: FAILED after 5000 ms - rc: 0, erc: 0x12345678
```

### Timing Metrics

The handler logs timing information to help diagnose performance issues:

**Cache Operations:**
```
[TIMING] MoveToUpdateCache: Processing 1 payload(s)
[TIMING] MoveToUpdateCache: File 0 cached in 2500 ms
[TIMING] MoveToUpdateCache: Completed 1 file(s) in 3000 ms total (copy time: 2500 ms)
```

**Pre-reboot Caching:**
```
[TIMING] CacheSourceUpdate: Starting pre-reboot cache operation
[TIMING] CacheSourceUpdate: Cache operation completed in 1500 ms
```

**Post-install Caching:**
```
[TIMING] OnUpdateWorkflowCompleted: Starting cache operation
[TIMING] OnUpdateWorkflowCompleted: Cache operation completed in 2000 ms (rc: 1, erc: 0x00000000)
```

### Monitoring Commands

**Real-time Delta Update Monitoring:**
```bash
# Watch all delta-related logs in real-time
journalctl -u deviceupdate-agent -f | grep -E "\[DELTA\]|\[TIMING\]"

# Watch for reconstruction events only
journalctl -u deviceupdate-agent -f | grep "\[DELTA\]"
```

**Check Delta Update Success Rate:**
```bash
# Count successful reconstructions
journalctl -u deviceupdate-agent | grep -c "\[DELTA\] Reconstruction SUCCESS"

# Count failed reconstructions (fell back to full download)
journalctl -u deviceupdate-agent | grep -c "\[DELTA\] Reconstruction FAILED"

# Count cache misses
journalctl -u deviceupdate-agent | grep -c "Source update not found in cache"
```

**Calculate Bandwidth Savings:**
```bash
# Extract bandwidth savings from logs
journalctl -u deviceupdate-agent | grep "\[DELTA\] Reconstruction SUCCESS" | \
  grep -oP "saved \K[0-9]+ bytes \([0-9]+%\)"
```

**Check Cache Health:**
```bash
# List cached source updates
ls -lh /var/lib/adu/cache/ /var/lib/adu/downloads/delta-cache/ 2>/dev/null

# Check cache metadata files
cat /var/lib/adu/cache/*/*.info 2>/dev/null
```

### Extended Result Codes (ERCs)

When delta operations fail, the logs include Extended Result Codes (ERCs) in hexadecimal format. Common ERCs:

| ERC Pattern | Meaning |
|-------------|---------|
| `ADUC_ERC_DDH_*` | Delta Download Handler errors |
| `ADUC_ERC_DDH_BAD_ARGS` | Invalid parameters passed to handler |
| `ADUC_ERC_DDH_SOURCE_UPDATE_CACHE_MISS` | Source file not in cache |
| `ADUC_ERC_DDH_PROCESSOR_*` | libadudiffapi processing errors |
| `ADUC_ERC_MOVE_COPYFALLBACK` | File copy failed during caching |
| `ADUC_ERC_MOVE_HASH_VERIFICATION_FAILED` | Cached file hash mismatch |

### Log Analysis Examples

**Successful Delta Update Flow:**
```
INFO  Update manifest contains downloadHandlerId: microsoft/delta:1
INFO  Loading download handler: libmicrosoft_delta_download_handler.so
INFO  [DELTA] Source update found in cache at '/var/lib/adu/cache/Contoso/sha256-abc123'
INFO  [DELTA] Starting reconstruction: source='/var/lib/adu/cache/...', delta='/var/lib/adu/downloads/...', target='/var/lib/adu/downloads/sandbox/...'
INFO  [DELTA] libadudiffapi apply succeeded - target file created at '/var/lib/adu/downloads/sandbox/v2.swu'
INFO  Processing Delta 0 succeeded
INFO  [DELTA] Reconstruction SUCCESS - Downloaded 52428800 bytes (delta) instead of 838860800 bytes (full), saved 786432000 bytes (93%)
INFO  DownloadHandlerPlugin ProcessUpdate result - rc: 700, erc: 0x00000000
INFO  Successfully reconstructed target file from delta and cached source update
```

**Failed Delta Update with Fallback:**
```
INFO  [DELTA] Source update not found in cache - cannot perform delta reconstruction
WARN  src update cache miss for Delta 0
WARN  [DELTA] Reconstruction FAILED for all 1 delta(s) - falling back to full download (838860800 bytes)
INFO  DownloadHandlerPlugin ProcessUpdate result - rc: 702, erc: 0x00000000
INFO  Starting full content download for: v2.swu
```

**Pre-reboot Caching Success:**
```
INFO  [TIMING] CacheSourceUpdate: Starting pre-reboot cache operation
INFO  updateCacheBasePath = NULL (will use default)
INFO  Calling ADUC_SourceUpdateCache_Move...
INFO  [TIMING] MoveToUpdateCache: Processing 1 payload(s)
INFO  File already cached at '/var/lib/adu/cache/...' with valid hash - skipping
INFO  [TIMING] MoveToUpdateCache: Completed 1 file(s) in 50 ms total (copy time: 0 ms)
INFO  [TIMING] CacheSourceUpdate: Cache operation completed in 55 ms
INFO  [TIMING] CacheSourceUpdate: SUCCESS - Source update cached in 55 ms before reboot
```

## Performance Considerations

- **Memory Usage**: Delta application may require memory proportional to file sizes
- **CPU Usage**: Compression/decompression is CPU-intensive
- **Disk I/O**: Temporary files created during delta application
- **Network**: Delta downloads are typically much smaller than full updates

## Security

- Handler validates all downloaded content using cryptographic hashes
- Delta library performs integrity checks during patch application
- Handler runs with ADU agent's security context
- All network communications use HTTPS/TLS

## Development and Debugging

### Enable Debug Logging

```bash
# Set log level in ADU configuration
sudo nano /etc/adu/du-config.json

# Add or modify:
{
  "logLevel": 3  # 0=Error, 1=Warning, 2=Info, 3=Debug
}

# Restart agent
sudo systemctl restart deviceupdate-agent
```

### Attach Debugger

```bash
# Find ADU agent process
ps aux | grep deviceupdate-agent

# Attach GDB (requires agent built with debug symbols)
sudo gdb -p <pid>

# Set breakpoints in handler code
break microsoft_delta_download_handler.c:function_name
```

### Build with Sanitizers

```bash
cd <adu-agent-root>/out
cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DADUC_BUILD_DELTA_HANDLER=ON \
    -DCMAKE_C_FLAGS="-fsanitize=address -fsanitize=undefined" \
    -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=undefined" \
    -G Ninja \
    ..
ninja
```

## Additional Resources

- [ADU Agent Documentation](../../../../../../docs/agent-reference/)
- [Delta Library Repository](https://github.com/Azure/iot-hub-device-update-delta)
- [ADU Service Documentation](https://docs.microsoft.com/azure/iot-hub-device-update/)
- [Extension Development Guide](../../../../../../docs/agent-reference/how-to-implement-custom-update-handler.md)

## Contributing

When modifying the delta download handler:

1. Follow the coding standards in `CONTRIBUTING.md`
2. Add unit tests for new functionality
3. Update this README with any new dependencies or procedures
4. Test with both Debug and Release builds
5. Verify package creation and installation
6. Test end-to-end with real delta updates

## License

This component is part of the Azure IoT Hub Device Update project and is licensed under the MIT License. See the LICENSE file in the repository root for details.
