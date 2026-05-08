# Test .deb Packages for APT Handler

Test packages used to validate the ADU Gen2 APT handler (`apt_handler_v2`).

## Quick Start

```bash
# Build test packages
./build_test_deb.sh

# Set up local APT repo (requires sudo)
./setup_local_repo.sh

# Test install/upgrade/remove
sudo apt-get install contoso-agent=1.0.0
sudo apt-get install contoso-agent=2.0.0
sudo apt-get remove contoso-agent
```

## Packages

| Package | Versions | Purpose |
|---------|----------|---------|
| `contoso-agent` | 1.0.0, 2.0.0 | Simulates a customer IoT agent |
| `contoso-libs` | 1.0.0, 2.0.0 | Dependency package for contoso-agent |

## What Gets Tested

- **Install**: Fresh package installation with dependencies
- **Upgrade**: Version upgrade (1.0.0 → 2.0.0) with dependency resolution
- **Remove**: Clean package removal
- **Pre/Post scripts**: postinst and prerm hooks execute correctly
- **Dependency chains**: contoso-agent depends on contoso-libs

## Files

- `build_test_deb.sh` — Builds all test .deb packages
- `setup_local_repo.sh` — Creates a local APT repo and configures apt sources
- `output/` — Built .deb files (generated, not committed)
- `repo/` — Local APT repository (generated, not committed)

## Notes

- `dpkg-deb --build` does not require root privileges
- `setup_local_repo.sh` requires sudo for apt source configuration
- The packages use `Architecture: all` so they work on any platform
