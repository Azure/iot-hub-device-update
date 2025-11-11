# Packer Resume/Incremental Build Quick Reference

## Problem
Packer builds take 30+ minutes. If they fail, you have to start over.

## Solution: Staged Builds

### First Time Setup
```bash
cd tools/packer
chmod +x packer-build.sh
packer init build-staged.pkr.hcl
```

### Build Workflow

#### Option 1: All at Once
```bash
./packer-build.sh all amd64 debian12
```

#### Option 2: Stage by Stage (Recommended for Development)
```bash
# Stage 1: Build dependencies (do once, ~30 min)
./packer-build.sh deps amd64 debian12

# Stage 2: Build code (fast, ~2-5 min)
./packer-build.sh build amd64 debian12

# Stage 3: Run tests (fast, ~1-2 min)
./packer-build.sh test amd64 debian12
```

### Resume from Failure

If build fails at any stage, just re-run that stage:
```bash
# Failed during build? Resume:
./packer-build.sh build amd64 debian12

# Failed during tests? Resume:
./packer-build.sh test amd64 debian12
```

### Iterate on Code Changes

After `deps` stage is cached:
```bash
# Edit code, then rebuild (fast!)
./packer-build.sh build amd64 debian12

# Rerun tests
./packer-build.sh test amd64 debian12
```

### Extract Packages
```bash
# Get .deb packages from final image
CONTAINER=$(docker create adu-delta-agent:debian12_amd64_final)
docker cp $CONTAINER:/iot-hub-device-update/out/. ./packages/
docker rm $CONTAINER

# View packages
ls -lh packages/*.deb
```

### Inspect Intermediate Images
```bash
# List all cached images
docker images | grep adu-delta-agent

# Run a shell in deps image (debug dependency issues)
docker run -it adu-delta-agent-deps:debian12_amd64_deps /bin/bash

# Run a shell in build image (debug build issues)
docker run -it adu-delta-agent-build:debian12_amd64_build /bin/bash
```

### Clean Up
```bash
# Remove all ADU images (free disk space)
docker rmi $(docker images -q 'adu-delta-agent*')

# Remove specific stage to force rebuild
docker rmi adu-delta-agent-deps:debian12_amd64_deps

# Remove only final images (keep cache)
docker rmi $(docker images -q 'adu-delta-agent:*')
```

## Supported Configurations

### Architectures
- `amd64` - x86_64 (Intel/AMD)
- `arm64` - ARM 64-bit (Raspberry Pi 4, etc.)

### Distributions
- `debian12` - Debian 12 Bookworm (latest)
- `debian11` - Debian 11 Bullseye
- `ubuntu2204` - Ubuntu 22.04 LTS
- `ubuntu2004` - Ubuntu 20.04 LTS

### Examples
```bash
# AMD64 builds
./packer-build.sh build amd64 debian12
./packer-build.sh build amd64 ubuntu2204

# ARM64 builds (requires QEMU)
./packer-build.sh build arm64 debian12
./packer-build.sh build arm64 ubuntu2204
```

## Time Savings

| Method | Initial | Rebuild | Test Only |
|--------|---------|---------|-----------|
| Single-stage | 35 min | 35 min | 35 min |
| Staged (cold) | 35 min | 2-5 min | 1-2 min |
| Staged (warm) | 0 min | 2-5 min | 1-2 min |

**Warm** = deps stage already cached

## Troubleshooting

### "Dependencies image not found"
```bash
# Run deps stage first
./packer-build.sh deps amd64 debian12
```

### "Build image not found"
```bash
# Run build stage first
./packer-build.sh build amd64 debian12
```

### Build still slow after deps cached
Check that install-deps.sh is detecting already-installed packages:
```bash
# Should show "✓ Already installed" messages
docker run adu-delta-agent-deps:debian12_amd64_deps bash -c \
  'cd /iot-hub-device-update && ./scripts/install-deps.sh --install-aduc-deps'
```

### Force complete rebuild
```bash
# Remove all cached images
docker rmi $(docker images -q 'adu-delta-agent*')

# Rebuild from scratch
./packer-build.sh all amd64 debian12
```
