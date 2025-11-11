#!/bin/bash
# Resume-friendly Packer build script
# This script builds in stages, allowing you to resume from any stage

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Default values
STAGE="${1:-all}"
ARCH="${2:-amd64}"
DISTRO="${3:-debian12}"

show_help() {
    cat << EOF
Usage: $(basename "$0") [STAGE] [ARCH] [DISTRO]

Build ADU Delta Agent in stages with resume capability.

STAGES:
  deps    - Build dependencies image (slowest, cache this!)
  build   - Build the project (requires deps stage)
  test    - Run tests and create final image (requires build stage)
  all     - Run all stages (default)

ARCH:
  amd64   - x86_64 architecture (default)
  arm64   - ARM64 architecture

DISTRO:
  debian12  - Debian 12 Bookworm (default)
  debian11  - Debian 11 Bullseye
  ubuntu2204 - Ubuntu 22.04
  ubuntu2004 - Ubuntu 20.04

Examples:
  # Full build
  $(basename "$0") all amd64 debian12

  # Build only dependencies (cache this!)
  $(basename "$0") deps amd64 debian12

  # Resume from build stage (skips deps)
  $(basename "$0") build amd64 debian12

  # Just run tests on existing build
  $(basename "$0") test amd64 debian12

After running 'deps' stage once, you can iterate quickly with:
  $(basename "$0") build    # Rebuild code
  $(basename "$0") test     # Rerun tests

EOF
}

if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    show_help
    exit 0
fi

# Validate inputs
case "$STAGE" in
    deps|build|test|all) ;;
    *) echo "Error: Invalid stage '$STAGE'. Use: deps, build, test, or all" >&2; show_help; exit 1 ;;
esac

case "$ARCH" in
    amd64|arm64) ;;
    *) echo "Error: Invalid arch '$ARCH'. Use: amd64 or arm64" >&2; exit 1 ;;
esac

TARGET="${DISTRO}_${ARCH}"
echo "======================================================================"
echo "Building ADU Delta Agent"
echo "  Stage:  $STAGE"
echo "  Arch:   $ARCH"
echo "  Distro: $DISTRO"
echo "  Target: $TARGET"
echo "======================================================================"
echo

# Check if staged config exists
if [[ -f "build-staged.pkr.hcl" ]]; then
    CONFIG_FILE="build-staged.pkr.hcl"
    echo "Using staged build configuration: $CONFIG_FILE"

    case "$STAGE" in
        deps)
            echo "Building dependencies image..."
            packer build -only="adu-deps.docker.${TARGET}_deps" "$CONFIG_FILE"
            echo
            echo "✓ Dependencies image created: adu-delta-agent-deps:${TARGET}_deps"
            echo "  You can now run: $(basename "$0") build $ARCH $DISTRO"
            ;;
        build)
            # Check if deps image exists
            if ! docker image inspect "adu-delta-agent-deps:${TARGET}_deps" >/dev/null 2>&1; then
                echo "Error: Dependencies image not found!"
                echo "Run first: $(basename "$0") deps $ARCH $DISTRO"
                exit 1
            fi
            echo "Building project (using cached dependencies)..."
            packer build -only="adu-build.docker.${TARGET}_build" "$CONFIG_FILE"
            echo
            echo "✓ Build image created: adu-delta-agent-build:${TARGET}_build"
            echo "  You can now run: $(basename "$0") test $ARCH $DISTRO"
            ;;
        test)
            # Check if build image exists
            if ! docker image inspect "adu-delta-agent-build:${TARGET}_build" >/dev/null 2>&1; then
                echo "Error: Build image not found!"
                echo "Run first: $(basename "$0") build $ARCH $DISTRO"
                exit 1
            fi
            echo "Running tests (using cached build)..."
            packer build -only="adu-final.docker.${TARGET}_final" "$CONFIG_FILE"
            echo
            echo "✓ Final image created: adu-delta-agent:${TARGET}_final"
            ;;
        all)
            echo "Running all stages..."
            echo
            echo "Stage 1/3: Dependencies..."
            $(basename "$0") deps "$ARCH" "$DISTRO"
            echo
            echo "Stage 2/3: Build..."
            $(basename "$0") build "$ARCH" "$DISTRO"
            echo
            echo "Stage 3/3: Test..."
            $(basename "$0") test "$ARCH" "$DISTRO"
            echo
            echo "✓ Complete build finished!"
            ;;
    esac
else
    # Fallback to original single-stage build
    echo "Using original single-stage build configuration"
    CONFIG_FILE="build.pkr.hcl"
    packer build -only="adu-delta-agent.docker.${TARGET}" "$CONFIG_FILE"
fi

echo
echo "======================================================================"
echo "Build complete!"
echo "======================================================================"
echo
echo "To extract packages from the image:"
echo "  CONTAINER=\$(docker create adu-delta-agent:${TARGET}_final)"
echo "  docker cp \$CONTAINER:/iot-hub-device-update/out/. ./packages/"
echo "  docker rm \$CONTAINER"
echo
echo "To list available images:"
echo "  docker images | grep adu-delta-agent"
echo
