# Multi-Architecture Build - Quick Reference

## Quick Start Commands

### GitHub Actions (Easiest)
```bash
git push origin your-branch
# Wait for CI to complete, download artifacts from Actions tab
```

### Packer Build (Local)
```bash
cd tools/packer
packer init .

# Build specific target
packer build -only='adu-delta-agent.docker.debian12_amd64' .

# Build all targets (takes time!)
packer build .
```

### Docker Build (Fast Local Test)
```bash
# AMD64
docker build --platform linux/amd64 -f .devcontainer/Dockerfile .

# ARM64 (requires QEMU)
docker buildx build --platform linux/arm64 -f .devcontainer/Dockerfile .
```

### Native Build
```bash
# Same as before, automatically detects architecture
./scripts/install-deps.sh --install-aduc-deps --install-do --install-cmake --install-delta
./scripts/build.sh --clean --build-packages --delta-handler
```

## Available Build Targets

| Target | Platform | Command |
|--------|----------|---------|
| Debian 12 AMD64 | Native or Docker | `debian12_amd64` |
| Debian 12 ARM64 | Native or Docker | `debian12_arm64` |
| Ubuntu 22.04 AMD64 | Native or Docker | `ubuntu2204_amd64` |
| Ubuntu 22.04 ARM64 | Native or Docker | `ubuntu2204_arm64` |

## Expected Output Packages

```
out/deviceupdate-agent_1.1.0_amd64.deb
out/deviceupdate-agent_1.1.0_arm64.deb
out/deviceupdate-agent-delta-handler_1.1.0_amd64.deb
out/deviceupdate-agent-delta-handler_1.1.0_arm64.deb
```

## Testing Packages

```bash
# Install and verify
sudo apt-get install -y ./out/deviceupdate-agent_*_$(dpkg --print-architecture).deb
which AducIotAgent
AducIotAgent --version

# Install delta handler
sudo apt-get install -y ./out/deviceupdate-agent-delta-handler_*.deb
ls -l /usr/lib/adu/extensions/sources/
```

## Common Issues

**ARM64 build slow?**
→ Normal with QEMU emulation, use native ARM64 runner if available

**"Exec format error"?**
→ Wrong architecture package, use `dpkg --print-architecture` to check

**Package installation fails?**
→ Use `apt-get install -y ./package.deb` to auto-resolve dependencies

## Documentation

- **Full Guide:** [docs/cross-compilation.md](cross-compilation.md)
- **Packer Details:** [tools/packer/README.md](../tools/packer/README.md)
- **Implementation:** [docs/IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)
