#!/bin/bash
# Build using LOCAL source code (no git clone)
# Perfect for rapid iteration without committing/pushing

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

ARCH="${1:-amd64}"
DISTRO="${2:-debian12}"

show_help() {
    cat << EOF
Usage: $(basename "$0") [ARCH] [DISTRO]

Build ADU Delta Agent using LOCAL source code (no git required).
Perfect for testing changes before committing.

ARCH:
  amd64   - x86_64 architecture (default)
  arm64   - ARM64 architecture

DISTRO:
  debian12   - Debian 12 Bookworm (default)
  debian11   - Debian 11 Bullseye
  ubuntu2204 - Ubuntu 22.04

Examples:
  # Build with local changes
  $(basename "$0") amd64 debian12

  # Test on different distro
  $(basename "$0") amd64 ubuntu2204

  # ARM64 build
  $(basename "$0") arm64 debian12

Features:
  ✓ Uses your LOCAL workspace files (uncommitted changes OK!)
  ✓ No need to commit/push to GitHub
  ✓ Fast iteration for development
  ✓ Complete build: deps + build + test

Note: This does a full build every time (no stage caching).
      For staged/cached builds, use packer-build.sh instead.

EOF
}

if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    show_help
    exit 0
fi

# Validate inputs
case "$ARCH" in
    amd64|arm64) ;;
    *) echo "Error: Invalid arch '$ARCH'. Use: amd64 or arm64" >&2; exit 1 ;;
esac

TARGET="${DISTRO}_${ARCH}_local"

echo "======================================================================"
echo "Building ADU Delta Agent (LOCAL SOURCE)"
echo "  Arch:   $ARCH"
echo "  Distro: $DISTRO"
echo "  Target: $TARGET"
echo "  Source: $(cd ../.. && pwd)"
echo "======================================================================"
echo
echo "⚠️  Using LOCAL source files (uncommitted changes will be included)"
echo

# Check if build-local.pkr.hcl exists
if [[ ! -f "build-local.pkr.hcl" ]]; then
    echo "Error: build-local.pkr.hcl not found in $(pwd)"
    exit 1
fi

# Initialize if needed
if [[ ! -d ".packer.d" ]]; then
    echo "Initializing Packer plugins..."
    packer init build-local.pkr.hcl
    echo
fi

# Build with local sources
echo "Building with Packer (this will take 30-45 minutes)..."
echo
packer build -only="adu-local.docker.${TARGET}" build-local.pkr.hcl

echo
echo "======================================================================"
echo "✓ Build complete!"
echo "======================================================================"
echo
echo "Docker image created: adu-delta-agent-local:${TARGET}"
echo
echo "To extract packages:"
echo "  CONTAINER=\$(docker create adu-delta-agent-local:${TARGET})"
echo "  docker cp \$CONTAINER:/iot-hub-device-update/out/. ./packages/"
echo "  docker rm \$CONTAINER"
echo
echo "To inspect the image:"
echo "  docker run -it adu-delta-agent-local:${TARGET} bash"
echo
echo "To view build artifacts inside container:"
echo "  docker run --rm adu-delta-agent-local:${TARGET} ls -lh /iot-hub-device-update/out/"
echo
