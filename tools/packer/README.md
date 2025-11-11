# Packer Tooling

This dir is for HCL configs used by [Hashicorp packer](https://www.packer.io/) for building VM and Docker images for various purposes.

## Quick Start

### Initialize Packer

First time setup:
```bash
cd tools/packer
packer init .
```

### Build All Architectures

```bash
packer build .
```

### Build Specific Target

```bash
# Debian 12 AMD64
packer build -only='adu-delta-agent.docker.debian12_amd64' .

# All ARM64 targets
packer build -only='adu-delta-agent.docker.*_arm64' .
```

## Resume Builds (Incremental/Staged Building)

For faster iteration and resumable builds, use the **staged build approach**:

### Option 1: Using the Helper Script (Recommended)

```bash
# Make script executable (first time only)
chmod +x packer-build.sh

# Build in stages (cache dependencies)
./packer-build.sh deps amd64 debian12      # Stage 1: Install dependencies (slow, but cached!)
./packer-build.sh build amd64 debian12     # Stage 2: Build code (fast, reuses deps)
./packer-build.sh test amd64 debian12      # Stage 3: Run tests (fast, reuses build)

# Or run all stages
./packer-build.sh all amd64 debian12

# After deps cached, iterate quickly on code changes:
./packer-build.sh build amd64 debian12     # Rebuild code only (~2-5 min)
./packer-build.sh test amd64 debian12      # Rerun tests only (~1-2 min)
```

**Available Options:**
- **Stages**: `deps`, `build`, `test`, `all`
- **Architectures**: `amd64`, `arm64`
- **Distros**: `debian12`, `debian11`, `ubuntu2204`, `ubuntu2004`

### Option 2: Direct Packer Commands

```bash
# Initialize staged config
packer init build-staged.pkr.hcl

# Stage 1: Build dependencies image (do this once)
packer build -only='adu-deps.docker.debian12_amd64_deps' build-staged.pkr.hcl

# Stage 2: Build project (reuses deps image)
packer build -only='adu-build.docker.debian12_amd64_build' build-staged.pkr.hcl

# Stage 3: Test and package (reuses build image)
packer build -only='adu-final.docker.debian12_amd64_final' build-staged.pkr.hcl
```

### Benefits of Staged Builds

✅ **Resume from any stage** - If build fails, restart from that stage
✅ **Cache dependencies** - 30+ min dependency build cached once
✅ **Fast iteration** - Code changes rebuild in 2-5 min
✅ **Debug friendly** - Inspect intermediate images
✅ **Disk efficient** - Reuse layers across builds

### Extract Build Artifacts

```bash
# From final image
CONTAINER=$(docker create adu-delta-agent:debian12_amd64_final)
docker cp $CONTAINER:/iot-hub-device-update/out/. ./packages/
docker rm $CONTAINER

# List all build images
docker images | grep adu-delta-agent
```

### Clean Up Cached Images

```bash
# Remove all ADU build images
docker rmi $(docker images -q 'adu-delta-agent*')

# Remove specific stage
docker rmi adu-delta-agent-deps:debian12_amd64_deps
```

## Files in This Directory

### build.pkr.hcl

Main multi-architecture build configuration supporting:
- Debian 12 (amd64, arm64)
- Debian 11 (amd64, arm64)
- Ubuntu 22.04 (amd64, arm64)
- Ubuntu 20.04 (amd64, arm64)

Single-stage build that completes everything in one run. Simple but slower for iteration.

**Usage:**
```bash
packer build -var="git_branch=your-branch" .
```

### build-staged.pkr.hcl

**NEW:** Multi-stage build configuration that splits the build into three cacheable stages:
1. **deps** - Install all dependencies (30+ min, but cached)
2. **build** - Build the project code (2-5 min with cache)
3. **test** - Run tests and create final image (1-2 min with cache)

This enables resumable builds and fast iteration on code changes.

**Usage:**
```bash
./packer-build.sh build amd64 debian12
```

### packer-build.sh

Helper script for staged builds. Provides easy resume functionality and validates dependencies between stages.

### build dir

The `build` dir contains legacy HCL for building docker images that will contain the build artifacts of building DU Agent, Unit tests, and .deb package during image creation time.
See README.md in `build` dir for more details.

## Supported Targets

| Distribution | Architecture | Build Target              |
|--------------|--------------|---------------------------|
| Debian 12    | amd64        | debian12_amd64           |
| Debian 12    | arm64        | debian12_arm64           |
| Debian 11    | amd64        | debian11_amd64           |
| Debian 11    | arm64        | debian11_arm64           |
| Ubuntu 22.04 | amd64        | ubuntu2204_amd64         |
| Ubuntu 22.04 | arm64        | ubuntu2204_arm64         |
| Ubuntu 20.04 | amd64        | ubuntu2004_amd64         |
| Ubuntu 20.04 | arm64        | ubuntu2004_arm64         |

## Prerequisites

### Quick Setup

Install all cross-compilation build tools in one command:
```bash
./scripts/install-deps.sh --xcompile-build-tools
```

This will install and configure:
- Docker (official Docker CE)
- HashiCorp Packer
- Docker user permissions (adds you to docker group)
- QEMU for multi-architecture support

After installation, you may need to start a new shell session or run:
```bash
newgrp docker  # Apply docker group membership
```

### Manual Installation

If you prefer manual setup:

1. **Install Docker**: https://docs.docker.com/engine/install/
   ```bash
   # Verify Docker is installed
   docker --version

   # Add user to docker group
   sudo usermod -aG docker $USER
   newgrp docker
   ```

2. **Install Packer**: https://developer.hashicorp.com/packer/install
   ```bash
   # Verify Packer is installed
   packer version
   ```

3. **Setup QEMU** (for ARM64 builds on AMD64 hosts):
   ```bash
   docker run --privileged --rm tonistiigi/binfmt --install all
   ```

## See Also

- [Multi-Architecture Build Guide](../../docs/cross-compilation.md)
- [GitHub Actions Workflow](../../.github/workflows/docker-build.yml)
