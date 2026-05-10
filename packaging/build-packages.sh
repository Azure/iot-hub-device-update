#!/usr/bin/env bash
#
# build-packages.sh — Build DEB or RPM packages for the ADU Gen2 agent.
#
# Usage:
#   ./build-packages.sh deb      Build Debian packages
#   ./build-packages.sh rpm      Build RPM packages
#   ./build-packages.sh auto     Auto-detect distro and build (default)
#
# Environment variables:
#   ADU_VERSION   — Override the package version (default: git tag or 2.0.0)
#   BUILD_DIR     — Override the build directory (default: _build_pkg)
#   JOBS          — Parallel build jobs (default: nproc)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# ── Version detection ────────────────────────────────────────────────

detect_version() {
    if [ -n "${ADU_VERSION:-}" ]; then
        echo "$ADU_VERSION"
        return
    fi

    # Try git tag (strip leading 'v' if present)
    local git_version
    if git_version="$(git -C "$SOURCE_DIR" describe --tags --abbrev=0 2>/dev/null)"; then
        echo "${git_version#v}"
        return
    fi

    echo "2.0.0"
}

VERSION="$(detect_version)"
BUILD_DIR="${BUILD_DIR:-${SOURCE_DIR}/_build_pkg}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"

echo "==> ADU Agent package build"
echo "    Version : $VERSION"
echo "    Source  : $SOURCE_DIR"
echo "    Build   : $BUILD_DIR"
echo "    Jobs    : $JOBS"
echo ""

# ── Distro detection ────────────────────────────────────────────────

detect_distro() {
    if [ -f /etc/os-release ]; then
        # shellcheck disable=SC1091
        . /etc/os-release
        case "$ID" in
            ubuntu|debian|linuxmint|pop)
                echo "debian"
                ;;
            fedora|rhel|centos|rocky|alma|ol)
                echo "rpm"
                ;;
            *)
                echo "unknown"
                ;;
        esac
    elif command -v dpkg > /dev/null 2>&1; then
        echo "debian"
    elif command -v rpm > /dev/null 2>&1; then
        echo "rpm"
    else
        echo "unknown"
    fi
}

# ── Build Debian packages ───────────────────────────────────────────

build_deb() {
    echo "==> Building Debian packages..."

    # Check for required tools
    for tool in dpkg-buildpackage debhelper cmake ninja-build; do
        case "$tool" in
            dpkg-buildpackage)
                if ! command -v dpkg-buildpackage > /dev/null 2>&1; then
                    echo "Error: dpkg-buildpackage not found. Install dpkg-dev." >&2
                    exit 1
                fi
                ;;
            debhelper)
                if ! dpkg -l debhelper > /dev/null 2>&1; then
                    echo "Error: debhelper not found. Install debhelper." >&2
                    exit 1
                fi
                ;;
            *)
                if ! command -v "$tool" > /dev/null 2>&1; then
                    echo "Error: $tool not found. Please install it." >&2
                    exit 1
                fi
                ;;
        esac
    done

    # Update changelog version
    local changelog="$SOURCE_DIR/packaging/debian/changelog"
    if [ -f "$changelog" ]; then
        sed -i "s/^adu-agent (.*)/adu-agent (${VERSION}-1)/" "$changelog"
    fi

    # Symlink debian/ to the source root if not already there
    if [ ! -e "$SOURCE_DIR/debian" ]; then
        ln -s packaging/debian "$SOURCE_DIR/debian"
    fi

    # Build
    cd "$SOURCE_DIR"
    dpkg-buildpackage -us -uc -b -j"$JOBS"

    echo ""
    echo "==> Debian packages built successfully."
    echo "    Packages are in: $(dirname "$SOURCE_DIR")/"
    ls -1 "$(dirname "$SOURCE_DIR")"/adu-*.deb 2>/dev/null || true
}

# ── Build RPM packages ──────────────────────────────────────────────

build_rpm() {
    echo "==> Building RPM packages..."

    # Check for required tools
    for tool in rpmbuild cmake ninja-build; do
        if ! command -v "$tool" > /dev/null 2>&1; then
            echo "Error: $tool not found. Please install it." >&2
            exit 1
        fi
    done

    local spec_file="$SOURCE_DIR/packaging/rpm/adu-agent.spec"

    # Create rpmbuild tree
    local rpmbuild_dir="$BUILD_DIR/rpmbuild"
    mkdir -p "$rpmbuild_dir"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

    # Create source tarball
    local tarball_name="adu-agent-${VERSION}"
    echo "    Creating source tarball..."
    tar czf "$rpmbuild_dir/SOURCES/${tarball_name}.tar.gz" \
        --transform="s|^$(basename "$SOURCE_DIR")|${tarball_name}|" \
        -C "$(dirname "$SOURCE_DIR")" \
        "$(basename "$SOURCE_DIR")"

    # Copy and update spec
    cp "$spec_file" "$rpmbuild_dir/SPECS/"
    sed -i "s/^Version:.*/Version:        ${VERSION}/" "$rpmbuild_dir/SPECS/adu-agent.spec"

    # Build
    rpmbuild \
        --define "_topdir $rpmbuild_dir" \
        --define "_jobs $JOBS" \
        -bb "$rpmbuild_dir/SPECS/adu-agent.spec"

    echo ""
    echo "==> RPM packages built successfully."
    echo "    Packages are in: $rpmbuild_dir/RPMS/"
    find "$rpmbuild_dir/RPMS" -name '*.rpm' -print 2>/dev/null || true
}

# ── Main ─────────────────────────────────────────────────────────────

usage() {
    echo "Usage: $0 {deb|rpm|auto}" >&2
    echo "" >&2
    echo "  deb   Build Debian packages (.deb)" >&2
    echo "  rpm   Build RPM packages (.rpm)" >&2
    echo "  auto  Auto-detect distro and build (default)" >&2
    exit 1
}

ACTION="${1:-auto}"

case "$ACTION" in
    deb)
        build_deb
        ;;
    rpm)
        build_rpm
        ;;
    auto)
        distro="$(detect_distro)"
        case "$distro" in
            debian)
                build_deb
                ;;
            rpm)
                build_rpm
                ;;
            *)
                echo "Error: Cannot detect distro family. Use '$0 deb' or '$0 rpm'." >&2
                exit 1
                ;;
        esac
        ;;
    -h|--help|help)
        usage
        ;;
    *)
        echo "Error: Unknown action '$ACTION'" >&2
        usage
        ;;
esac
