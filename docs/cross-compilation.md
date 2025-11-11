# Multi-Architecture Build Guide

This document describes how to build the Azure Device Update Agent for multiple architectures (amd64, arm64) across different distributions (Debian 11/12, Ubuntu 20.04/22.04).

## Table of Contents

- [Overview](#overview)
- [Supported Platforms](#supported-platforms)
- [Build Methods](#build-methods)
  - [Method 1: GitHub Actions (Recommended)](#method-1-github-actions-recommended)
  - [Method 2: Packer Multi-Arch Build](#method-2-packer-multi-arch-build)
  - [Method 3: Local Docker Build](#method-3-local-docker-build)
  - [Method 4: Native Cross-Compilation](#method-4-native-cross-compilation)
- [Architecture Detection](#architecture-detection)
- [Package Naming Convention](#package-naming-convention)
- [Testing Multi-Arch Packages](#testing-multi-arch-packages)
- [Troubleshooting](#troubleshooting)

## Overview

The ADU Agent supports multiple build approaches for creating architecture-specific packages:

1. **Native builds per architecture** - Most reliable, used in CI/CD
2. **Emulated builds with QEMU** - For testing without native hardware
3. **Cross-compilation** - Advanced, requires toolchain setup

**Recommended Approach:** Native builds using GitHub Actions or Packer with Docker multi-platform support.

## Supported Platforms

| Distribution | Architecture | Status | Notes |
|--------------|--------------|--------|-------|
| Debian 12    | amd64        | ✅ Tested | Primary target |
| Debian 12    | arm64        | ✅ Tested | Primary target |
| Debian 11    | amd64        | ✅ Tested | |
| Debian 11    | arm64        | ✅ Tested | |
| Ubuntu 22.04 | amd64        | ✅ Tested | |
| Ubuntu 22.04 | arm64        | ✅ Tested | |
| Ubuntu 20.04 | amd64        | ✅ Tested | |
| Ubuntu 20.04 | arm64        | ✅ Tested | |
| Debian 10    | amd64        | ⚠️ Legacy | Excluded from matrix |

## Build Methods

### Method 1: GitHub Actions (Recommended)

GitHub Actions provides the most reliable builds using native architecture runners.

**Workflow:** `.github/workflows/docker-build.yml`

```yaml
strategy:
  matrix:
    os: ["debian:12", "debian:11", "ubuntu:22.04", "ubuntu:20.04"]
    arch: [amd64, arm64]
runs-on: ${{ matrix.arch == 'arm64' && 'ubuntu-22.04-arm' || 'ubuntu-latest' }}
container:
  image: ${{ matrix.os }}
  options: --platform linux/${{ matrix.arch }}
```

**Features:**
- ✅ Native builds on architecture-specific runners
- ✅ Automatic artifact upload
- ✅ Package installation testing
- ✅ Matrix strategy for all combinations
- ✅ No QEMU emulation overhead

**Trigger a Build:**
```bash
git push origin your-branch
# Or manually trigger
gh workflow run docker-build.yml
```

**Download Artifacts:**
- GitHub UI: Actions tab → Select run → Artifacts section
- GitHub CLI: `gh run download <run-id>`

### Method 2: Packer Multi-Arch Build

HashiCorp Packer provides reproducible multi-architecture Docker image builds.

**Prerequisites:**
```bash
# Install Packer
brew install packer  # macOS
# or
wget https://releases.hashicorp.com/packer/1.9.4/packer_1.9.4_linux_amd64.zip
unzip packer_*.zip
sudo mv packer /usr/local/bin/

# Enable multi-arch support (for ARM64 on AMD64 host)
docker run --privileged --rm tonistiigi/binfmt --install all
```

**Initialize and Build:**
```bash
cd tools/packer

# First time setup
packer init .

# Build all architectures and distributions
packer build .

# Build specific target
packer build -only='adu-delta-agent.docker.debian12_amd64' .

# Build all ARM64 targets
packer build -only='adu-delta-agent.docker.*_arm64' .

# Build with custom branch
packer build -var="git_branch=main" -var="container_tag=v1.2.3" .
```

**Extract Build Artifacts:**
```bash
# Get image ID
docker images adu-delta-agent

# Run container
docker run -d --name extract adu-delta-agent:debian12_amd64 sleep infinity

# Copy packages
docker cp extract:/iot-hub-device-update/out/. ./packages/

# Cleanup
docker stop extract && docker rm extract
```

**Available Targets:**
- `debian12_amd64`, `debian12_arm64`
- `debian11_amd64`, `debian11_arm64`
- `ubuntu2204_amd64`, `ubuntu2204_arm64`
- `ubuntu2004_amd64`, `ubuntu2004_arm64`

See [tools/packer/README.md](../tools/packer/README.md) for detailed Packer usage.

### Method 3: Local Docker Build

Quick local builds using Docker Buildx for multi-platform support.

**Prerequisites:**
```bash
# Enable Docker Buildx
docker buildx create --name multiarch --use
docker buildx inspect --bootstrap

# Install QEMU for ARM64 emulation on AMD64
docker run --privileged --rm tonistiigi/binfmt --install all
```

**Build for Specific Architecture:**
```bash
# AMD64 build (native)
docker build --platform linux/amd64 -f .devcontainer/Dockerfile -t adu-agent:amd64 .

# ARM64 build (emulated on AMD64, or native on ARM64)
docker buildx build --platform linux/arm64 -f .devcontainer/Dockerfile -t adu-agent:arm64 .

# Multi-arch build and push
docker buildx build --platform linux/amd64,linux/arm64 -t myregistry/adu-agent:latest --push .
```

**Run Build Inside Container:**
```bash
# Start container for specific architecture
docker run -it --platform linux/arm64 debian:12 bash

# Inside container:
git clone <repo-url>
cd iot-hub-device-update
./scripts/install-deps.sh --install-aduc-deps --install-do --install-cmake --install-delta
./scripts/build.sh --clean --build-packages --delta-handler
```

**Performance Notes:**
- Native builds: ~10-15 minutes
- QEMU emulated (ARM64 on AMD64): ~45-90 minutes
- Use native runners when possible!

### Method 4: Native Cross-Compilation

Cross-compilation builds ARM64 binaries on AMD64 host without emulation.

⚠️ **Advanced:** This method requires careful toolchain configuration and is **not recommended** unless you have specific requirements.

**Install Cross-Compilation Toolchain:**
```bash
# On Debian/Ubuntu AMD64 host
sudo apt-get install -y \
  gcc-aarch64-linux-gnu \
  g++-aarch64-linux-gnu \
  binutils-aarch64-linux-gnu

# Install ARM64 system libraries
sudo dpkg --add-architecture arm64
sudo apt-get update
sudo apt-get install -y \
  libcurl4-openssl-dev:arm64 \
  libssl-dev:arm64 \
  uuid-dev:arm64
```

**Create CMake Toolchain File:**
```cmake
# toolchain-arm64.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

**Build with Cross-Compilation:**
```bash
mkdir build-arm64
cd build-arm64
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-arm64.cmake -GNinja ..
ninja
cpack
```

**Challenges:**
- ❌ Library path complications
- ❌ Dependencies must be available for target architecture
- ❌ Some dependencies don't support cross-compilation
- ❌ Testing requires ARM64 hardware or QEMU
- ❌ CMake find_package() issues with cross builds

**When to Use:**
- You need fastest possible ARM64 builds on AMD64 (no emulation overhead)
- You have a working cross-compilation environment already
- You're building for embedded systems with custom toolchains

**Recommendation:** Use native builds or QEMU emulation instead. The complexity is not worth the speed gain for most use cases.

## Architecture Detection

The build system automatically detects the target architecture.

### CMake Detection

In `CMakeLists.txt`:
```cmake
if (NOT DEFINED CMAKE_SYSTEM_PROCESSOR)
    execute_process (
        COMMAND uname -m
        OUTPUT_VARIABLE SYS_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    set (CMAKE_SYSTEM_PROCESSOR ${SYS_ARCH} CACHE STRING "System processor architecture")
endif ()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set (IS_ARM64 TRUE)
    message (STATUS "Building for ARM64")
endif ()
```

### Shell Script Detection

In `scripts/install-deps.sh`:
```bash
determine_machine_architecture() {
    arch="$(uname -m)"
    case $arch in
        x86_64|amd64)
            export ARCH=amd64
            export IS_ARM64=false
            ;;
        aarch64|arm64)
            export ARCH=arm64
            export IS_ARM64=true
            ;;
    esac
    echo "Detected architecture: $ARCH"
}
```

### Verification

Check detected architecture:
```bash
# During build
./scripts/build.sh --clean --build-packages | grep "System processor"

# In running system
uname -m                    # x86_64 or aarch64
dpkg --print-architecture   # amd64 or arm64
```

## Package Naming Convention

All Debian packages automatically include the architecture in their filename.

### Package File Format

```
<package-name>_<version>_<architecture>.deb
```

### Examples

Main agent package:
- `deviceupdate-agent_1.1.0_amd64.deb`
- `deviceupdate-agent_1.1.0_arm64.deb`

Delta handler package (when built with `--delta-handler`):
- `deviceupdate-agent-delta-handler_1.1.0_amd64.deb`
- `deviceupdate-agent-delta-handler_1.1.0_arm64.deb`

### CMake Configuration

From `packages/CMakeLists.txt`:
```cmake
# DEB-DEFAULT automatically generates architecture-specific file names
set (CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
```

This uses CPack's default naming which includes `CMAKE_SYSTEM_PROCESSOR`.

### Verify Package Architecture

```bash
# List packages
ls -lh out/*.deb

# Check package metadata
dpkg-deb --info deviceupdate-agent_1.1.0_arm64.deb | grep Architecture
# Output: Architecture: arm64

# Query installed package
dpkg -s deviceupdate-agent | grep Architecture
```

## Testing Multi-Arch Packages

### Installation Testing

The CI workflow automatically tests package installation:

```bash
# Install main agent
apt-get install -y ./out/deviceupdate-agent_*.deb

# Verify binaries
which AducIotAgent
which adu-shell

# Verify libraries
test -f /usr/lib/adu/libadushconst_exports.so

# Verify configuration
test -f /etc/adu/du-config.json

# Install delta handler (if available)
apt-get install -y ./out/deviceupdate-agent-delta-handler_*.deb
test -f /usr/lib/adu/extensions/sources/libmicrosoft_delta_download_handler.so
```

### Manual Testing on Target Architecture

**Test on ARM64 Device (e.g., Raspberry Pi):**
```bash
# Transfer package
scp out/deviceupdate-agent_*_arm64.deb pi@raspberrypi:~/

# SSH to device
ssh pi@raspberrypi

# Install
sudo apt-get update
sudo apt-get install -y ~/deviceupdate-agent_*_arm64.deb

# Verify
AducIotAgent --version
systemctl status deviceupdate-agent
```

**Test in QEMU Container:**
```bash
# Run ARM64 container on AMD64 host
docker run -it --platform linux/arm64 debian:12 bash

# Inside container, copy and test package
apt-get update && apt-get install -y ./deviceupdate-agent_*_arm64.deb
AducIotAgent --version
```

### Compatibility Testing Matrix

Test each package on its intended architecture:

| Package | Test Platform | Expected Result |
|---------|---------------|-----------------|
| `*_amd64.deb` | Debian 12 AMD64 | ✅ Install succeeds |
| `*_amd64.deb` | Debian 12 ARM64 | ❌ Wrong architecture error |
| `*_arm64.deb` | Debian 12 ARM64 | ✅ Install succeeds |
| `*_arm64.deb` | Debian 12 AMD64 | ❌ Wrong architecture error |

## Troubleshooting

### Issue: ARM64 Build Extremely Slow

**Symptom:** ARM64 builds take 10x longer than AMD64
**Cause:** QEMU emulation overhead
**Solution:**
1. Use native ARM64 runners (GitHub Actions ARM64 runners)
2. Build on actual ARM64 hardware
3. Accept the slowdown for occasional builds

### Issue: "cannot execute binary file: Exec format error"

**Symptom:** Binary fails to run with exec format error
**Cause:** Wrong architecture binary for the host
**Solution:**
```bash
# Check binary architecture
file /usr/bin/AducIotAgent
# Should match system
uname -m

# Reinstall correct package
apt-get remove deviceupdate-agent
apt-get install ./deviceupdate-agent_*_$(dpkg --print-architecture).deb
```

### Issue: QEMU Not Working for ARM64 Builds

**Symptom:** `standard_init_linux.go: exec user process caused "exec format error"`
**Solution:**
```bash
# Reinstall QEMU emulators
docker run --privileged --rm tonistiigi/binfmt --uninstall qemu-*
docker run --privileged --rm tonistiigi/binfmt --install all

# Verify
docker run --rm --platform linux/arm64 debian:12 uname -m
# Should output: aarch64
```

### Issue: Package Installation Fails with Missing Dependencies

**Symptom:** `dpkg: dependency problems prevent configuration`
**Solution:**
```bash
# Install with apt-get to auto-resolve dependencies
apt-get install -y ./deviceupdate-agent_*.deb

# Or manually install dependencies first
apt-get update
apt-get install -y deliveryoptimization-agent libdeliveryoptimization curl
```

### Issue: Cross-Compilation Fails to Find Libraries

**Symptom:** CMake can't find libcurl, libssl for ARM64
**Solution:**
```bash
# Enable multi-arch
sudo dpkg --add-architecture arm64
sudo apt-get update

# Install ARM64 libraries
sudo apt-get install -y libcurl4-openssl-dev:arm64 libssl-dev:arm64

# Set CMake search paths
cmake -DCMAKE_FIND_ROOT_PATH=/usr/aarch64-linux-gnu ...
```

### Issue: Packer Build Hangs or Times Out

**Symptom:** Packer provisioners hang during dependency installation
**Solution:**
```bash
# Increase timeout in build.pkr.hcl
provisioner "shell" {
  timeout = "60m"
  inline = [...]
}

# Or reduce parallelism
packer build -parallel-builds=1 .
```

### Issue: Package Architecture Mismatch

**Symptom:** Built package has wrong architecture in filename
**Solution:**
```bash
# Verify build environment
uname -m
dpkg --print-architecture

# Force architecture in Docker
docker run --platform linux/arm64 ...

# Check CMake detection
cmake . | grep "System processor"
```

## Performance Comparison

Build time comparison for full build + packaging on typical hardware:

| Method | Platform | Architecture | Time | Notes |
|--------|----------|--------------|------|-------|
| Native | AMD64 Host | amd64 | ~12 min | Fastest |
| Native | ARM64 Host | arm64 | ~15 min | Pi 4: ~45 min |
| GitHub Actions | AMD64 Runner | amd64 | ~15 min | Includes setup |
| GitHub Actions | ARM64 Runner | arm64 | ~18 min | |
| QEMU Emulation | AMD64 Host | arm64 | ~60 min | 4-5x slower |
| Cross-Compile | AMD64 Host | arm64 | ~15 min | ⚠️ Complex setup |
| Packer | AMD64 Host | amd64 | ~20 min | Includes image build |
| Packer | AMD64 Host | arm64 | ~75 min | QEMU overhead |

**Recommendation:** Use native builds whenever possible. GitHub Actions with ARM64 runners provides the best balance of speed, reliability, and convenience.

## See Also

- [Packer Build Configuration](../tools/packer/README.md)
- [GitHub Actions Workflow](../.github/workflows/docker-build.yml)
- [Building ADU Agent](agent-reference/how-to-build-agent-code.md)
- [Docker Multi-Platform Builds](https://docs.docker.com/build/building/multi-platform/)
- [Debian Package Architecture](https://www.debian.org/doc/debian-policy/ch-controlfields.html#s-f-architecture)
