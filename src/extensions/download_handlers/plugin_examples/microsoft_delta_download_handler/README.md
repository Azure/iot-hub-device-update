# Microsoft Delta Download Handler

## Overview

The Microsoft Delta Download Handler is an extension for the Azure Device Update (ADU) agent that enables efficient delta-based firmware and software updates. Instead of downloading complete update files, this handler processes delta patches that contain only the differences between the current and target versions, significantly reducing bandwidth usage and update time.

## Purpose

This download handler:
- Downloads and processes delta update files from Azure Device Update service
- Applies binary delta patches to existing files on the device
- Validates downloaded content and applied patches
- Integrates seamlessly with the ADU agent's update workflow
- Supports various compression and delta algorithms through the `libadudiffapi` library

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

### Delta Download Fails

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
1. Ensure sufficient disk space for download and processing
2. Check network connectivity and firewall rules
3. Verify ADU service credentials in configuration
4. Check delta file integrity on the service side

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
