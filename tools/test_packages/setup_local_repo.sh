#!/bin/bash
# Set up a local apt repository from test .deb packages
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="${SCRIPT_DIR}/output"
REPO_DIR="${SCRIPT_DIR}/repo"

if [ ! -d "$OUTPUT_DIR" ] || [ -z "$(ls $OUTPUT_DIR/*.deb 2>/dev/null)" ]; then
    echo "No .deb packages found. Building..."
    bash "$SCRIPT_DIR/build_test_deb.sh"
fi

echo "Setting up local APT repository..."

rm -rf "$REPO_DIR"
mkdir -p "$REPO_DIR"
cp "$OUTPUT_DIR"/*.deb "$REPO_DIR/"

cd "$REPO_DIR"
dpkg-scanpackages . /dev/null 2>/dev/null | gzip -9c > Packages.gz

# Create sources.list entry
SOURCES_FILE="/etc/apt/sources.list.d/adu-test-packages.list"
echo "deb [trusted=yes] file://$REPO_DIR ./" | sudo tee "$SOURCES_FILE" > /dev/null

sudo apt-get update -qq 2>/dev/null

echo ""
echo "✓ Local APT repository configured"
echo "  Repo: $REPO_DIR"
echo "  Source: $SOURCES_FILE"
echo ""
echo "Available packages:"
apt-cache show contoso-agent 2>/dev/null | grep -E "^(Package|Version):" | head -6
echo ""
echo "Test with:"
echo "  apt-get install -y contoso-agent=1.0.0"
echo "  apt-get install -y contoso-agent=2.0.0  # upgrade"
echo "  apt-get remove -y contoso-agent          # remove"
