#!/bin/bash
# Prepare debug symbols for symbol server upload
# This script collects .debug files and creates a symbols package
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${ROOT_DIR}/out"
SYMBOLS_DIR="${BUILD_DIR}/symbols"
PACKAGE_DIR="${BUILD_DIR}/symbols-package"

echo "ADU Gen2 — Symbol Package Builder"
echo "================================="

if [ ! -d "$SYMBOLS_DIR" ]; then
    echo "ERROR: No symbols directory found. Build with -DADUC_STRIP_SYMBOLS=ON first."
    echo "  cmake -DADUC_STRIP_SYMBOLS=ON -S . -B out && ninja -C out && ninja -C out collect_symbols"
    exit 1
fi

# Create package structure (mimics symsrv layout)
rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR"

echo ""
echo "Processing symbols..."

for debugfile in "$SYMBOLS_DIR"/*.debug; do
    if [ ! -f "$debugfile" ]; then
        continue
    fi

    basename=$(basename "$debugfile" .debug)

    # Extract build-id
    build_id=$(readelf -n "$debugfile" 2>/dev/null | grep "Build ID" | awk '{print $3}')

    if [ -z "$build_id" ]; then
        echo "  WARN: No build-id for $basename, using filename hash"
        build_id=$(sha256sum "$debugfile" | cut -c1-40)
    fi

    # Create symsrv-style path: symbols/<name>/<build-id>/<name>.debug
    dest_dir="$PACKAGE_DIR/$basename/$build_id"
    mkdir -p "$dest_dir"
    cp "$debugfile" "$dest_dir/$basename.debug"

    echo "  ✓ $basename (build-id: ${build_id:0:16}...)"
done

# Create manifest
cat > "$PACKAGE_DIR/manifest.json" << EOF
{
    "product": "adu-gen2-agent",
    "version": "$(git -C "$ROOT_DIR" describe --tags --always 2>/dev/null || echo "dev")",
    "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
    "platform": "linux-$(uname -m)",
    "symbols": $(find "$PACKAGE_DIR" -name "*.debug" | wc -l)
}
EOF

echo ""
echo "Symbol package ready: $PACKAGE_DIR"
echo "  Files: $(find "$PACKAGE_DIR" -name "*.debug" | wc -l) symbols"
echo "  Size:  $(du -sh "$PACKAGE_DIR" | cut -f1)"
echo ""
echo "To upload to symsrv:"
echo "  symstore add /r /f \"$PACKAGE_DIR\" /s \\\\server\\symbols /t \"ADU-Gen2\""
echo ""
echo "To upload to Azure Artifacts:"
echo "  az artifacts universal publish --feed adu-symbols --name adu-gen2-symbols \\"
echo "    --version \$(git describe) --path \"$PACKAGE_DIR\""
