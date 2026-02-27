# How To Build the Device Update Agent

This guide provides comprehensive instructions for building the Device Update agent, including dependency management, platform-specific guidance, and customization options.

## Quick Navigation

-   [Dependencies Overview](#dependencies-of-device-update-agent) - Complete dependency reference
-   [Platform Compatibility](#platform-compatibility-matrix) - Supported platforms and versions
-   [Installation Guide](#installing-dependencies) - Automated dependency installation
-   [Build Scenarios](#dependency-matrix-by-build-scenario) - Choose the right build for your needs
-   [Platform Instructions](#platform-specific-instructions) - Ubuntu, Debian, Yocto guidance
-   [Environment Caching](#environment-caching-and-configuration-sharing) - Shared configuration between scripts
-   [Troubleshooting](#troubleshooting-dependencies) - Common issues and solutions
-   [Building Process](#building-the-device-update-agent-for-linux) - Actual build steps
-   [As a standalone solution](#as-a-standalone-solution) - Integration approaches
-   [Dependency Strategy](#dependency-build-strategy-and-cross-platform-considerations) - Why we build from source

## Dependencies of Device Update Agent

This section provides a comprehensive overview of all dependencies required to build and run the Device Update agent. Dependencies are organized by category with explanations of their purpose and requirements.

### System Package Dependencies

The following system packages are required for building the agent. These are automatically installed by [`scripts/install-deps.sh`](../../scripts/install-deps.sh).

#### Core Build Tools
| Package | Purpose | Version Required |
|---------|---------|------------------|
| `build-essential` | Essential compilation tools (gcc, g++, libc6-dev, make) | Latest available |
| `cmake` | Cross-platform build system generator | 3.5+ |
| `ninja-build` | Fast parallel build tool (alternative to make) | Latest available |
| `make` | Build automation tool | Latest available |
| `git` | Version control for downloading source dependencies | Latest available |
| `pkg-config` | Helper tool for compiling applications and libraries | Latest available |

#### Compiler Requirements
| Package | Purpose | Version Required |
|---------|---------|------------------|
| `gcc` / `g++` | GNU Compiler Collection | 6.3+ (7.4+ recommended) |
| Alternative: `clang` | LLVM C/C++ compiler | 6.0+ |

**Platform-specific compiler versions:**
- **Debian 9**: gcc-6, g++-6 (6.3+) **(NO LONGER SUPPORTED as of 10/24/2025)**
- **Ubuntu 18.04+**: gcc-8, g++-8 (7.4+ available by default, 8+ installed by script) **(NO LONGER SUPPORTED as of 10/24/2025)**
- **Debian 11**: gcc-10, g++-10
- **Debian 12**: gcc-12, g++-12
- **Ubuntu 20.04/22.04**: gcc-10, g++-10

#### Network and Security Libraries
| Package | Purpose | Required For |
|---------|---------|--------------|
| `libcurl4-openssl-dev` | HTTP/HTTPS client library | Download operations, web requests |
| `libssl-dev` | OpenSSL development headers | Cryptographic operations, TLS/SSL |
| `curl` | Command-line download utility | Script operations |
| `wget` | Web file retrieval utility | Dependency downloads |

#### System Utilities
| Package | Purpose | Required For |
|---------|---------|--------------|
| `uuid-dev` | UUID generation library | Unique identifier generation |
| `libxml2-dev` | XML parsing library | Configuration and manifest parsing |
| `lsb-release` | Linux Standard Base information | OS version detection |

### External Source Dependencies

These major components are built from source during the build process:

#### Azure IoT C SDK
- **Repository**: [Azure/azure-iot-sdk-c](https://github.com/Azure/azure-iot-sdk-c)
- **Purpose**: Connect to IoT Hub and call Azure IoT Plug and Play APIs
- **Default Branch**: `LTS_08_2023`
- **Required For**: All Azure IoT Hub communication (MQTT, device authentication, telemetry)
- **Customization**: Use `--azure-iot-sdk-ref <branch/tag>` to specify version

#### Delivery Optimization SDK
- **Repository**: [microsoft/do-client](https://github.com/microsoft/do-client)
- **Purpose**: Robust, efficient download mechanism for update packages
- **Default Branch**: `develop`
- **Required For**: Update package downloads (can be disabled with curl fallback)
- **Customization**: Use `--do-ref <branch/tag>` to specify version

#### Azure Blob Storage File Upload Utility
- **Purpose**: Upload files to Azure storage (logs, diagnostics)
- **Required For**: Diagnostic data upload functionality

#### IotHub Device Update Delta
- **Purpose**: Delta update functionality for efficient incremental updates
- **Required For**: Advanced update scenarios with delta compression

### Development and Testing Dependencies

#### Testing Framework
| Package | Purpose | Installation |
|---------|---------|--------------|
| `Catch2` | C++ unit testing framework | Built from source |
| **Default Version**: `v2.13.9` | Unit test execution | `--catch2-ref <version>` to customize |

#### Static Analysis Tools (Optional)
| Package | Purpose | Installation |
|---------|---------|--------------|
| `clang` | C/C++ compiler and analyzer | `apt install clang` |
| `clang-tidy` | Clang-based linter | `apt install clang-tidy` |
| `cppcheck` | Static analysis tool | `apt install cppcheck` |
| `clang-format` | Code formatting | `apt install clang-format` |

#### Additional Development Tools (Optional)
| Tool | Purpose | Installation |
|------|---------|--------------|
| `cmake-format` | CMake file formatting | `pip3 install cmake-format` |
| `shellcheck` | Shell script linting | Auto-installed by script |
| `doxygen` | Documentation generation | `apt install doxygen` (for `--build-documentation`) |
| `graphviz` | Graph visualization | `apt install graphviz` (for documentation) |

### Optional Platform-Specific Dependencies

#### Optional Platform-Specific Dependencies

#### SWUpdate Handler (Ubuntu)
- **Purpose**: Support for SWUpdate-based system updates
- **Installation**: `--install-swupdate`
- **Default Version**: Latest from [sbabic/swupdate](https://github.com/sbabic/swupdate)
- **Required Libraries**: `libconfig-dev` (auto-installed)

### Dependency Matrix by Build Scenario

The following table shows which dependencies are required for different build and deployment scenarios:

| Dependency Category | Minimal Build | Production Build | Development | Testing | Documentation |
|---------------------|:-------------:|:----------------:|:-----------:|:-------:|:-------------:|
| **Core Build Tools** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Compiler (GCC/Clang)** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Network Libraries** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **System Utilities** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Azure IoT C SDK** | ✅ | ✅ | ✅ | ✅ | ❌ |
| **Delivery Optimization** | ❌* | ✅ | ✅ | ✅ | ❌ |
| **Azure Storage SDK** | ❌ | ✅ | ✅ | ❌ | ❌ |
| **Catch2 Testing** | ❌ | ❌ | ✅ | ✅ | ❌ |
| **Static Analysis** | ❌ | ❌ | ✅ | ✅ | ❌ |
| **SWUpdate** | ❌ | ❌** | ✅ | ✅ | ❌ |
| **Documentation Tools** | ❌ | ❌ | ❌ | ❌ | ✅ |

**Legend:**
- ✅ Required
- ❌ Not needed
- ❌* Optional (can use curl fallback)
- ❌** Optional (only if using SWUpdate updates)

#### Installation Commands by Scenario

| Scenario | Command | Purpose |
|----------|---------|---------|
| **Quick Start** | `./scripts/install-deps.sh -a` | Everything needed for development |
| **Minimal Build** | `./scripts/install-deps.sh --install-packages-only` | System packages only |
| **Production** | `./scripts/install-deps.sh --install-aduc-deps --install-do --install-packages` | Core runtime dependencies |
| **Development** | `./scripts/install-deps.sh -a --keep-source-code` | Full setup with source preservation |
| **CI/Testing** | `./scripts/install-deps.sh -a --install-githooks` | Complete with testing tools |
| **Documentation** | `sudo apt install doxygen graphviz` | Documentation generation only |

## Building the Device Update Agent for Linux

### Installing pkg-config (Required for SDK Usage)

If you plan to use the ADU SDK in external applications, you'll need pkg-config installed first:

#### Ubuntu/Debian
```sh
sudo apt update
sudo apt install pkgconfig
```

#### RHEL/Fedora (Community Support)
```sh
# RHEL 7
sudo yum install pkgconfig

# RHEL 8+ / Fedora
sudo dnf install pkgconfig
```

#### Yocto/Embedded Linux
Add to your image recipe:
```bitbake
IMAGE_INSTALL_append = " pkgconfig"
```

### Platform Compatibility Matrix

The Device Update agent has been tested and validated on the following platforms:

| Distribution | Version | Architecture | Compiler | Status | Notes |
|--------------|---------|--------------|----------|--------|-------|
| **Ubuntu** | 18.04 LTS | x64, ARM32, ARM64 | GCC 7.4+ (8+ installed) | ✅ Supported | Minimum supported version |
| **Ubuntu** | 20.04 LTS | x64, ARM32, ARM64 | GCC 9.4+ | ✅ Supported | Recommended |
| **Ubuntu** | 22.04 LTS | x64, ARM32, ARM64 | GCC 11+ | ✅ Supported | Latest tested |
| **Debian** | 9 (Stretch) | x64, ARM32, ARM64 | GCC 6.3+ | ✅ Supported | Legacy support |
| **Debian** | 10 (Buster) | x64, ARM32, ARM64 | GCC 8.3+ | ✅ Supported | Stable |
| **Debian** | 11 (Bullseye) | x64, ARM32, ARM64 | GCC 10.2+ | ✅ Supported | Recommended |
| **Debian** | 12 (Bookworm) | x64, ARM32, ARM64 | GCC 12+ | ✅ Supported | Latest tested |
| **RHEL** | 8+ | x64, ARM64 | GCC 8+ | 🟡 Community | Manual setup required |
| **Fedora** | 33+ | x64, ARM64 | GCC 10+ | 🟡 Community | Manual setup required |

**Status Legend:**
- ✅ **Supported**: Fully tested with automated installation and official support
- 🟡 **Community**: Community-supported, may work but requires manual setup
- ❌ **Not Supported**: Known compatibility issues

#### Platform-Specific Considerations

##### Ubuntu/Debian (Officially Supported)
- **Auto-detection**: Script automatically detects version and installs appropriate compiler
- **Package Manager**: Full apt integration with dependency resolution
- **Testing**: Primary CI/CD platform with extensive validation
- **Support**: Official Microsoft support available

##### RHEL/Fedora (Community Support)
- **Manual Setup**: Requires manual installation of build tools and dependencies
- **Package Differences**: Some package names differ from Debian-based distributions
- **Compiler**: May need to install newer GCC versions manually
- **Support**: Community-supported, not officially supported by Microsoft

```sh
# RHEL 8+
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake openssl-devel libcurl-devel libuuid-devel
```

##### Yocto/OpenEmbedded
- **Build Host**: Run dependency installation on build host, not target
- **Target Integration**: Include required runtime libraries in target image
- **Cross-compilation**: Ensure proper toolchain configuration

```bitbake
# Example Yocto recipe additions
DEPENDS += "openssl curl util-linux cmake-native"
IMAGE_INSTALL_append = " \
    openssl \
    curl \
    util-linux-libuuid \
    pkgconfig \
"
```

#### Architecture-Specific Notes

##### ARM32 (armhf)
- **Memory**: Minimum 256MB RAM recommended (128MB minimum)
- **Compiler**: Use GCC 6.3+ for compatibility, GCC 8+ recommended for optimal performance
- **Testing**: Extensively tested on Raspberry Pi 3/4

##### ARM64 (aarch64)
- **Performance**: Recommended for production ARM deployments
- **Compatibility**: Full feature parity with x64 builds
- **Testing**: Validated on various ARM64 SBCs and cloud instances

##### x64 (amd64)
- **Standard**: Primary development and testing platform
- **Performance**: Best performance for development and high-throughput scenarios

### Installing Dependencies

The Device Update agent provides a comprehensive dependency installation script that handles all the complexity of installing dependencies across different platforms and scenarios.

#### Quick Start - Install All Dependencies

For most users, this single command installs everything needed:

```sh
./scripts/install-deps.sh -a
```

This is equivalent to running:
```sh
./scripts/install-deps.sh --install-aduc-deps --install-do --install-packages --install-cmake --install-shellcheck
```

#### Script Options and Customization

The [`scripts/install-deps.sh`](../../scripts/install-deps.sh) script provides extensive customization options for different build scenarios.

**Environment Caching**: The script automatically loads previous settings from `.adu-dev/build.env` if available, and caches current settings after successful installation. Command-line options always override cached defaults.

##### Primary Installation Categories

| Option | Purpose | Includes |
|--------|---------|----------|
| `-a, --install-all-deps` | **Complete installation** (recommended) | All categories below |
| `--install-aduc-deps` | Agent core dependencies | Azure IoT SDK, Catch2, system packages |
| `--install-do` | Delivery Optimization | DO SDK from source |
| `-p, --install-packages` | System packages only | apt packages listed above |
| `--install-packages-only` | System packages without source builds | Package dependencies only |

##### Individual Component Options

| Option | Purpose | Default Version | Customization |
|--------|---------|-----------------|---------------|
| `--install-azure-iot-sdk` | Azure IoT C SDK | `LTS_08_2023` | `--azure-iot-sdk-ref <branch>` |
| `--install-do` | Delivery Optimization SDK | `develop` | `--do-ref <branch/tag>` |
| `--install-azure-storage-sdk` | Azure SDK for C++ | Latest | For blob storage features |
| `--install-catch2` | Testing framework | `v2.13.9` | `--catch2-ref <version>` |
| `--install-cmake` | Build system | Platform-specific | `--cmake-version <version>` |
| `--install-shellcheck` | Shell script linting | Latest | Auto-managed |
| `--install-swupdate` | SWUpdate support | Latest | `--swupdate-ref <version>` |
| `--install-githooks` | Repository git hooks | N/A | Development workflow |

##### Configuration Options

| Option | Purpose | Default | Example |
|--------|---------|---------|---------|
| `-f, --work-folder <path>` | Source code location | `/tmp` | `~/adu-deps` |
| `-k, --keep-source-code` | Preserve source after build | Delete | Keep for debugging |
| `--use-ssh` | Use SSH for git clones | HTTPS | For authenticated repos |
| `--cmake-prefix <path>` | CMake install location | `/tmp` | `/usr/local` |
| `--cmake-force-source` | Force CMake from source | Installer first | Override detection |
| `--list-deps` | Show dependency status | N/A | Diagnostic tool |

#### Common Build Scenarios

##### Minimal Build (Packages Only)
```sh
# Install only system packages, use existing dependencies
./scripts/install-deps.sh --install-packages-only
```

##### Development Environment
```sh
# Full installation with source preservation for debugging
./scripts/install-deps.sh -a -f ~/adu-dev-deps --keep-source-code
```

##### Custom IoT SDK Version
```sh
# Use specific Azure IoT SDK branch
./scripts/install-deps.sh --install-aduc-deps --azure-iot-sdk-ref v1.10.0
```

##### Testing Environment
```sh
# Install with SWUpdate support for handler testing
./scripts/install-deps.sh -a --install-swupdate
```

##### Continuous Integration
```sh
# Install with githooks for automated builds
./scripts/install-deps.sh -a --install-githooks
```

##### Corporate Environment (SSH)
```sh
# Use SSH for repositories requiring authentication
./scripts/install-deps.sh -a --use-ssh
```

## Environment Caching and Configuration Sharing

The ADU build system automatically caches environment variables and build configuration between `install-deps.sh` and `build.sh` to ensure consistent builds and simplify the development workflow.

### How Environment Caching Works

When you run `install-deps.sh`, it:

1. **Loads previous settings** from `.adu-dev/build.env` if available (e.g., work folder, SDK versions, compiler paths)
2. **Caches current configuration** after successful installation to `.adu-dev/build.env`
3. **Shares environment variables** with `build.sh` for consistent builds

When you run `build.sh`, it:

1. **Automatically loads** cached environment from `.adu-dev/build.env` if available
2. **Uses cached values as defaults** (work folder, cmake paths, compiler settings)
3. **Allows command-line overrides** of any cached values

### Cached Environment Variables

The following variables are automatically shared between scripts:

#### Build Directories and Paths
- `ADUC_WORK_FOLDER` - Source code location for dependencies
- `ADUC_CMAKE_DIR_PATH` - CMake installation directory
- `CMAKE_PREFIX` - CMake install prefix

#### Compiler and Tool Settings
- `CC`, `CXX` - Primary compiler paths
- `CATCH2_CC`, `CATCH2_CXX` - Catch2-specific compiler paths
- `CMAKE_BIN` - CMake binary path
- `CMAKE_VERSION` - Installed CMake version

#### SDK and Library References
- `AZURE_IOT_SDK_REF` - Azure IoT SDK version/branch
- `CATCH2_REF` - Catch2 testing framework version
- `SWUPDATE_REF` - SWUpdate version
- `DO_REF` - Delivery Optimization version

#### System Information
- `ADUC_OS`, `ADUC_VERSION` - Operating system details
- `ADUC_IS_AMD64`, `ADUC_IS_ARM64`, `ADUC_IS_ARM32` - Architecture flags

### Benefits of Environment Caching

1. **Consistency**: Ensures `build.sh` uses the same paths and versions as `install-deps.sh`
2. **Convenience**: No need to repeatedly specify custom work folders or tool paths
3. **Reliability**: Reduces configuration drift between dependency installation and building
4. **Flexibility**: Command-line options always override cached values

### Managing the Build Environment Cache

#### View Current Cache
```sh
# Display cached environment variables
cat .adu-dev/build.env
```

#### Reset Build Environment
```sh
# Method 1: Delete the entire cache directory
rm -rf .adu-dev/

# Method 2: Delete just the cache file
rm -f .adu-dev/build.env

# Method 3: Override with fresh installation
./scripts/install-deps.sh -a --work-folder /tmp/fresh-build
```

#### Override Cached Values
```sh
# install-deps.sh: Command-line options override cached defaults
./scripts/install-deps.sh -a --work-folder ~/custom-deps  # Overrides cached work folder

# build.sh: Command-line options override cached defaults
./scripts/build.sh -o ~/custom-output  # Overrides cached output directory
```

#### Troubleshooting Cache Issues

##### Stale Cache After System Changes
```sh
# If you've moved directories or changed system configuration
rm -f .adu-dev/build.env
./scripts/install-deps.sh -a  # Recreates cache with current settings
```

##### Debugging Cache Loading
```sh
# Both scripts show cache loading messages:
# "Loading cached build environment from .adu-dev/build.env..."
# "Cached environment loaded successfully."

# If cache loading fails, check file permissions
ls -la .adu-dev/build.env
```

##### Cache Location and Portability
- **Cache Location**: `.adu-dev/build.env` (relative to repository root)
- **Git Ignore**: Cache directory is automatically ignored by git
- **Portability**: Cache is machine-specific and should not be shared between systems

#### Platform-Specific Instructions

##### Ubuntu/Debian (Officially Supported)
```sh
# Standard installation
sudo apt update
./scripts/install-deps.sh -a
```

##### RHEL/Fedora (Community Support)
```sh
# Install pkg-config first (see platform compatibility section for details)
sudo dnf install pkgconfig  # RHEL 8+/Fedora
./scripts/install-deps.sh -a  # May require manual intervention
```

##### Yocto/Embedded Linux
```sh
# Install pkg-config in your image recipe
IMAGE_INSTALL_append = " pkgconfig"
# Then run script on target or build host
```

#### Troubleshooting Dependencies

##### Authentication Issues
- **GitHub 2FA**: Use personal access token (PAT) instead of password
- **SSH Keys**: Use `--use-ssh` option for SSH-based authentication
- **Corporate Proxy**: Configure git and curl proxy settings

##### Permission Issues
```sh
# Script will prompt for sudo when needed
# Ensure user has sudo privileges for package installation
```

##### Dependency Conflicts
```sh
# List current dependency status
./scripts/install-deps.sh --list-deps

# Clean rebuild with custom work folder
./scripts/install-deps.sh -a -f ~/clean-build --work-folder ~/clean-build
```

##### Version Conflicts
```sh
# Force specific versions
./scripts/install-deps.sh --install-aduc-deps \
    --azure-iot-sdk-ref LTS_01_2024 \
    --catch2-ref v2.13.9
```

### Keeping Documentation in Sync

This documentation is designed to stay synchronized with the [`scripts/install-deps.sh`](../../scripts/install-deps.sh) implementation. To ensure accuracy:

#### For Contributors
When modifying `install-deps.sh`, please update this documentation:

1. **Package Lists**: Update the [System Package Dependencies](#system-package-dependencies) tables
2. **Script Options**: Update the [Script Options and Customization](#script-options-and-customization) section
3. **Version Defaults**: Update default versions for external dependencies
4. **Platform Support**: Update the [Platform Compatibility Matrix](#platform-compatibility-matrix)

#### Validation Commands
Use these commands to verify documentation accuracy:

```sh
# Check current script help text
./scripts/install-deps.sh -h
./scripts/build.sh -h

# List dependency status
./scripts/install-deps.sh --list-deps

# Verify package list (compare with documentation)
grep -n "aduc_packages=" scripts/install-deps.sh
grep -n "static_analysis_packages=" scripts/install-deps.sh

# Test environment caching functionality
./scripts/install-deps.sh --install-packages-only --work-folder /tmp/test
cat .adu-dev/build.env  # Verify cache contents
rm -f .adu-dev/build.env  # Reset for testing
```

#### Quick Reference
Key locations in `install-deps.sh` that should match documentation:

| Documentation Section | Script Location | Line(s) |
|----------------------|-----------------|---------|
| [System Package Dependencies](#system-package-dependencies) | `aduc_packages=` | ~91 |
| [Static Analysis Tools](#development-and-testing-dependencies) | `static_analysis_packages=` | ~92 |
| [Script Options](#script-options-and-customization) | `print_help()` function | ~102-145 |
| [Default Versions](#external-source-dependencies) | Variable definitions | ~50-90 |
| [Environment Caching](#environment-caching-and-configuration-sharing) | `cache_build_environment()` function | ~1217-1265 |
| [Environment Loading](#environment-caching-and-configuration-sharing) | Cache loading at startup | ~53-58 |

### Install Optional Development Tools

- Install the clang-format package (required for running `scripts/clang-format.sh`):

```sh
sudo apt install clang-format
```

- Install pip3 and cmake-format (required for running `scripts/cmake-format.sh`):

```sh
sudo apt install python3-pip
sudo --set-home pip3 install cmake-format
```

### Device Update Linux Build System

The Device Update for IoT Hub reference agent code utilizes CMake for building. An example build script is provided at [scripts/build.sh](../../scripts/build.sh).

#### Build Using build.sh

The build script automatically integrates with the environment caching system. If you've run `install-deps.sh`, the build script will automatically use the cached environment settings.

To build the reference agent with the default parameters:

```sh
./scripts/build.sh -c
```

**Environment Integration**: The script automatically loads cached variables from `.adu-dev/build.env` if available, ensuring consistent compiler paths, work directories, and CMake settings from your dependency installation.

To see additional build options with build.sh:

```sh
build.sh -h
```

#### Build Script Environment Features

The build script provides several environment-aware features:

- **Automatic Cache Loading**: Loads build environment from `.adu-dev/build.env` if available
- **Cached CMake Path**: Uses CMake installed by `install-deps.sh` automatically
- **Compiler Consistency**: Uses the same compiler settings as dependency installation
- **Work Folder Integration**: Aligns with dependency installation work folder settings

#### Common Build Patterns

```sh
# Standard build (uses cached environment if available)
./scripts/build.sh -c

# Build with unit tests (leverages cached Catch2 installation)
./scripts/build.sh -c -u

# Clean build with documentation
./scripts/build.sh -c -d

# Override cached output directory
./scripts/build.sh -c -o ~/custom-output
```

### Build and Run the unit tests

To build and run the unit tests:

```sh
./scripts/build.sh -c -u
pushd out
ctest # or ninja test
```

For more test run options:

```sh
ctest -h
```

### Run the Unit Tests under Valgrind

The current supported valgrind is 3.19.0 and can be built from sources via:

```sh

```

There is a top-level `DartConfiguration.tcl` in the source tree that contains valgrind path and arguments.
Running memcheck with following command will result in CTest generating a `DartConfiguration.tcl` under the `out` dir and running all the tests using valgrind:

```sh
cd out
ctest -T memcheck
```

No suppression file is currently used, so the goal is for all the unit tests to run valgrind-clean and to fix even the false-positives.

### Build the Debian package

To build the debian package (will be output to the `out` directory):

```sh
./scripts/build.sh --build-packages
```

## Building the ADU SDK Library

The ADU SDK provides a C API for external applications to query the Azure Device Update agent service status. This is particularly useful for IoT devices that need to:

- Determine if the agent is actively processing updates
- Safely power down during idle periods to conserve battery
- Monitor update deployment workflow status

### Build the SDK Library

The SDK is built as part of the main build process and produces:
- **Library**: `libaducsdk.a` (static library)
- **Header**: `aducsdk.h` (C/C++ header file)
- **pkg-config**: `aducsdk.pc` (package configuration for discovery)

To build just the SDK:

```sh
./scripts/build.sh -c
# or build only the SDK target
cmake --build out --target aducsdk
```

### Install the SDK

To install the SDK for system-wide use:

```sh
sudo cmake --build out --target install
```

This installs:
- Library: `/usr/local/lib/libaducsdk.a`
- Header: `/usr/local/include/aduc/aducsdk.h`
- pkg-config: `/usr/local/lib/pkgconfig/aducsdk.pc`

### Configuring SDK Build Options

#### FIFO Path Configuration

Configure the default FIFO path for communication with the agent:

```sh
# Custom FIFO path
cmake -DADUC_API_DEFAULT_FIFO_PATH="/custom/path/to/api/apireq.fifo" ..
./scripts/build.sh -c
```

### Using the SDK in External Applications

#### Using pkg-config (Recommended)

```sh
# Check if SDK is installed
pkg-config --exists aducsdk && echo "SDK found!"

# Get compilation flags
gcc myapp.c $(pkg-config --cflags --libs aducsdk) -o myapp

# Check version
pkg-config --modversion aducsdk
```

#### Example Application

```c
#include <stdio.h>
#include <aduc/aducsdk.h>

int main() {
    printf("Checking ADU Agent status...\n");

    ADUC_ServiceStatus status = GetAduServiceStatus();
    const char* statusStr = ADUC_ServiceStatusToString(status);

    printf("Status: %s (%d)\n", statusStr, status);

    // Power management logic for IoT device
    if (status == ADUC_ServiceStatus_Idle || status == ADUC_ServiceStatus_Paused) {
        printf("Agent is idle/paused - safe to power down to conserve battery\n");
        // system("poweroff");  // Uncomment for actual power management
    } else if (status >= ADUC_ServiceStatus_ERROR_UnsupportedApiVersion) {
        printf("Error communicating with agent: %s\n", statusStr);
        return 1;
    } else {
        printf("Agent is active - staying online\n");
    }

    return 0;
}
```

#### Using CMake

In your `CMakeLists.txt`:

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(ADUCSDK REQUIRED aducsdk)

target_include_directories(myapp PRIVATE ${ADUCSDK_INCLUDE_DIRS})
target_link_libraries(myapp ${ADUCSDK_LIBRARIES})
target_compile_options(myapp PRIVATE ${ADUCSDK_CFLAGS_OTHER})
```

### Yocto Integration

#### In your Yocto recipe (e.g., `myapp_1.0.bb`):

```bitbake
DESCRIPTION = "IoT Power Management Application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=..."

# Add dependency on the ADU SDK
DEPENDS += "aducsdk"

# Use pkg-config to get compilation flags
inherit pkgconfig

do_compile() {
    # pkg-config automatically provides the right flags
    ${CC} ${CFLAGS} $(pkg-config --cflags aducsdk) -o myapp main.c $(pkg-config --libs aducsdk)
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 myapp ${D}${bindir}/
}
```

#### Overriding SDK Configuration in Yocto

To customize SDK build parameters in Yocto, add to your recipe or `local.conf`:

```bitbake
# Set custom FIFO path
EXTRA_OECMAKE_append = " -DADUC_API_DEFAULT_FIFO_PATH='/custom/adu/api/apireq.fifo'"

# Both together
EXTRA_OECMAKE_append = " -DADUC_API_DEFAULT_FIFO_PATH='/opt/adu/api/request.fifo'"
```

Or in your device-specific configuration:

```bitbake
# In your machine configuration (.conf file)
ADUC_API_DEFAULT_FIFO_PATH = "/custom/path/apireq.fifo"
```

### Build the agent using CMake

Alternatively, you can build using CMake directly. Set the required product values
for ADUC_DEVICEINFO_MANUFACTURER and ADUC_DEVICEINFO_MODEL in the top-level
[CMakeLists.txt](../../CMakeLists.txt) before building. Optional CMake values can be found there as well.

```sh
mkdir -p build && pushd build
cmake ..
cmake --build .
popd > /dev/null
```

or using Ninja

```sh
mkdir -p build && pushd build
cmake -G Ninja ..
ninja
popd > /dev/null
```

You can do incremental builds with Ninja:

```sh
pushd out && ninja
popd
```

### Use Curl for RootKey Package Download instead of Delivery Optimization Agent (DO)

One can already configure the agent to use curl to download update payload content (by registering curl content downloader extension, `/var/lib/adu/extensions/sources/libcurl_content_downloader.so`).

However, to switch to curl for downloading the RootKey Package (infrastructure file for update signature verification), one must currently rebuild the agent from sources by providing the `--rootkeypkg-curl` parameter to [build.sh](../../scripts/build.sh).

For example:

```sh
./scripts/build.sh -c --rootkeypkg-curl
```

Notes:

- Just as with update payload downloads, using curl for downloads will require `/usr/bin/curl` to be available on the device that has the AducIotAgent binary.

- curl will be invoked with the following cmd-line: `/usr/bin/curl -L -C - -o /path/to/output/file`

### Build Options for MQTT and MQTT over WebSockets IotHub Transport Providers

#### Allow MQTT or MQTT over Websockets IotHub Transport Protocols Driven from Config

By Default, both mqtt and mqtt/WebSockets will be linked into the AducIotAgent binary agent and driven by the `"iotHubProtocol"` config property with valid values of `"mqtt"` or `"mqtt/ws"`, for MQTT and MQTT over WebSockets, respectively.

Here is the default in top-level `CMakeLists.txt`:

```sh
set (
    ADUC_IOT_HUB_PROTOCOL
    "IotHub_Protocol_from_Config"
    CACHE
        STRING
        "The protocol for Azure IotHub SDK communication. Options are MQTT, MQTT_over_WebSockets, and IotHub_Protocol_from_Config")
```

Sample `/etc/adu/du-config.json` that selects `MQTT` value for the `iotHubProtocol` property:

```json
{
  ...
  "iotHubProtocol": "mqtt"
}
```

Sample `/etc/adu/du-config.json` that selects `MQTT over WebSockets` value for the `iotHubProtocol` property:

```json
{
  ...
  "iotHubProtocol": "mqtt/ws"
}
```

#### Use only MQTT IotHub Transport Protocol

If using only `MQTT`, then choosing `MQTT` for `ADUC_IOT_HUB_PROTOCOL` in the top-level `CMakeLists.txt` will reduce the size of Type=SizeMinRel AducIotAgent binary by about 60 KB, which is relatively small compared to the overall footprint that is on the order of 2-3 megabytes.

To link in only MQTT transport provider, set this in the top-level `CMakeLists.txt`:

```sh
set (
    ADUC_IOT_HUB_PROTOCOL
    "MQTT"
    CACHE
        STRING
        "The protocol for Azure IotHub SDK communication. Options are MQTT, MQTT_over_WebSockets, and IotHub_Protocol_from_Config")
```

#### Use only MQTT over WebSockets IotHub Transport Protocol

After enabling WebSockets above using install-deps.sh so that it builds libiothub_client_mqtt_ws_transport.a static library, modify the top-level CMakeLists.txt to use MQTT_over_WebSockets:

```sh
set (
    ADUC_IOT_HUB_PROTOCOL
    "MQTT_over_WebSockets"
    CACHE
        STRING
        "The protocol for Azure IotHub SDK communication. Options are MQTT, MQTT_over_WebSockets, and IotHub_Protocol_from_Config")
```

Doing ./build.sh after setting this to `"MQTT_over_WebSockets"` will have the MQTT traffic go over a websocket on port `443`.
Using `"MQTT"` will use SecureMQTT over port `8883`.

Please note that, by default, both MQTT (`libiothub_client_mqtt_transport.a`) and MQTT over WebSockets(`libiothub_client_mqtt_ws_transport.a`) static libraries are built by `install-deps.sh` via the `use_mqtt` and `use_wsio` -D configs.

```sh
# Verify mqtt transport static lib exists
$ locate libiothub_client_mqtt_transport.a | \
    grep '/usr/local/lib/'
/usr/local/lib/libiothub_client_mqtt_transport.a
```

```sh
# Verify mqtt over websockets static lib exists
$ locate libiothub_client_mqtt_ws_transport.a | \
    grep '/usr/local/lib/'
/usr/local/lib/libiothub_client_mqtt_ws_transport.a
```

## Install the Device Update Agent

### Install the Device Update Agent after building

```sh
sudo cmake --build out --target install
```

or using Ninja

```sh
pushd out > /dev/null
sudo ninja install
popd > /dev/null
```

**Note** If the Device Update Agent was built as a daemon, the install targets will install and register the Device Update Agent as a daemon.

### Install the Device Update Agent and Extensions from Debian Package

After building the Debian package using `build.sh --build-packages`, do:

```sh
sudo apt install ./out/{PKG_NAME}.deb
```

````markdown

## Dependency Build Strategy and Cross-Platform Considerations

This section explains the rationale behind the ADU project's approach to dependency management, particularly for CMake and other build-from-source dependencies.

### CMake Version Strategy

The ADU project requires CMake 3.23.2 rather than relying on system package managers for several important reasons:

#### Version Requirements
- **Project minimum**: Most ADU components require CMake 3.5+
- **Tool requirements**: Some tools require CMake 3.22+ (`tools/download_file`)
- **Azure SDK compatibility**: Azure Storage SDK requires CMake 3.13+
- **Target version**: 3.23.2 ensures compatibility with all components and modern features

#### Cross-Platform & Architecture Support

The install script uses a smart strategy for CMake installation:

```bash
# Supported architectures: Use pre-built installers (fast)
if [[ $is_amd64 == "true" || $is_arm64 == "true" ]]; then
    # Download official CMake installer from GitHub releases
    # Faster installation, pre-tested binaries
    download_cmake_installer_3.23.2
else
    # Unsupported architectures: Build from source
    # Ensures compatibility with RISC-V, ARM32, etc.
    build_cmake_from_source
fi
```

**Architecture Coverage:**
- ✅ **x86_64 (amd64)**: Pre-built installer (fastest)
- ✅ **aarch64 (ARM64)**: Pre-built installer (Raspberry Pi 4, AWS Graviton)
- ✅ **ARM32, RISC-V, others**: Built from source (IoT/embedded targets)

#### Cross-Compilation Benefits

Building CMake from source enables:

1. **Consistent Toolchain**: Same CMake version across all target platforms
2. **Embedded Device Support**: IoT devices with custom architectures
3. **Container Reproducibility**: Identical builds in Docker, CI/CD
4. **Distro Independence**: Works across Ubuntu, Debian, Alpine, Yocto

#### CI/CD and Build Consistency

**Problem with system packages:**
```bash
# Inconsistent versions across distributions
Ubuntu 20.04: cmake 3.16.3   # Too old for some tools
Ubuntu 22.04: cmake 3.22.1   # Close but not identical
Debian 11:    cmake 3.18.4   # Different feature set
```

**Solution with controlled installation:**
```bash
# Identical version everywhere
All platforms: cmake 3.23.2  # Guaranteed compatibility
```

### Built-from-Source Dependencies Strategy

The ADU project builds several key dependencies from source for similar cross-platform reliability:

#### Azure IoT C SDK (LTS_08_2023)
- **Reason**: Specific LTS branch with known stability
- **Benefit**: Consistent Azure connectivity across all platforms
- **Alternative**: System packages often have different versions/patches

#### Catch2 Testing Framework (v3.8.0)
- **Reason**: Specific version ensures test compatibility
- **Benefit**: Identical test behavior in CI and local development
- **Alternative**: System packages may not have the exact version needed

#### Delivery Optimization (develop branch)
- **Reason**: Latest features for Microsoft's DO client
- **Benefit**: Cutting-edge download optimization
- **Alternative**: Not available in most system package repositories

#### Azure Storage SDK (azure-core_1.6.0)
- **Reason**: Specific tag for blob storage features
- **Benefit**: Known-good version for ADU's storage requirements
- **Alternative**: System packages significantly behind latest releases

### Architecture-Specific Considerations

#### x86_64 / amd64 Systems
- **Primary target**: Development machines, cloud VMs
- **Strategy**: Pre-built binaries when available, source builds for consistency
- **Performance**: Optimized for rapid development cycles

#### ARM64 / aarch64 Systems
- **Primary target**: Raspberry Pi 4+, AWS Graviton, Apple Silicon
- **Strategy**: Pre-built ARM64 binaries, native compilation
- **Performance**: Excellent native performance on modern ARM

#### ARM32 / armv7l Systems
- **Primary target**: Raspberry Pi 3, older embedded systems
- **Strategy**: Cross-compilation or native source builds
- **Considerations**: Memory constraints, longer build times

#### RISC-V and Emerging Architectures
- **Primary target**: Future IoT devices, research platforms
- **Strategy**: Source builds ensure forward compatibility
- **Benefit**: ADU ready for next-generation hardware

### Recommendations for Different Use Cases

#### Development Environment
```bash
# Fast setup for development
./scripts/install-deps.sh --install-all-deps
# Uses pre-built binaries where possible
```

#### Production/Embedded Build
```bash
# Explicit control for production
./scripts/install-deps.sh --install-all-deps --cmake-force-source
# Ensures exact same build environment as CI
```

#### Cross-Compilation Setup
```bash
# Target-specific build
./scripts/install-deps.sh --install-all-deps \
  --work-folder ./target-deps \
  --keep-source-code yes
# Preserves source for cross-compilation investigation
```

#### Minimal CI Environment
```bash
# Container-optimized
./scripts/install-deps.sh --install-packages-only
# Use pre-installed CMake in container
```

### Performance and Storage Considerations

| Approach | Build Time | Disk Usage | Reproducibility | Cross-Platform |
|----------|------------|------------|-----------------|----------------|
| System packages | Fastest (minutes) | Minimal | Poor | Limited |
| Pre-built binaries | Fast (minutes) | Moderate | Good | Good |
| Source builds | Slow (30+ min) | High | Excellent | Excellent |
| **ADU hybrid** | **Balanced** | **Reasonable** | **Excellent** | **Excellent** |

### Future Considerations

As the ADU project evolves, the dependency strategy may be updated to:

1. **CMake 3.25+**: For improved C++20 support and performance
2. **Conan integration**: For more sophisticated dependency management
3. **Multi-stage containers**: For optimized production deployments
4. **vcpkg improvements**: Leveraging Microsoft's package manager enhancements

This approach ensures ADU remains buildable and reliable across the diverse landscape of IoT devices and development environments.

## Run Device Update Agent
````

Run Device Update Agent by following these [instructions](./how-to-run-agent.md)

### Verify MQTT Port

If using MQTT, run the following netstat command to verify that it is connecting to the remote MQTT port of 8883:

```sh
$ sudo ./out/bin/AducIotClient -l0 -e > /dev/null 2>&1 &
$ sudo netstat -pantu | grep Adu
tcp        0      0 <LOCAL IP ADDR>:<LOCAL PORT>       <REMOTE IP ADDR>:8883         ESTABLISHED <PID>/./out/bin/Aduc
$ fg
<ctrl-c>
```

### Verify MQTT over WebSockets Port

If using MQTT over WebSockets, run the following netstat command to verify that it is connecting to the remote WebSockets port of 443:

```sh
$ sudo ./out/bin/AducIotClient -l0 -e > /dev/null 2>&1 &
$ sudo netstat -pantu | grep Adu
tcp        0      0 <LOCAL IP ADDR>:<LOCAL PORT>       <REMOTE IP ADDR>:443         ESTABLISHED <PID>/./out/bin/Aduc
$ fg
<ctrl-c>
```
