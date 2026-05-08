#!/bin/bash
# Build test .deb packages for APT handler integration testing
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="${SCRIPT_DIR}/output"
VERSION="${1:-1.0.0}"

echo "Building test .deb packages (version: $VERSION)..."

rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# ═══════════════════════════════════════════════════
# Package 1: contoso-agent (simulates customer agent)
# ═══════════════════════════════════════════════════
PKG_DIR="$OUTPUT_DIR/contoso-agent_${VERSION}"
mkdir -p "$PKG_DIR/DEBIAN"
mkdir -p "$PKG_DIR/usr/bin"
mkdir -p "$PKG_DIR/etc/contoso"

# Control file
cat > "$PKG_DIR/DEBIAN/control" << EOF
Package: contoso-agent
Version: $VERSION
Section: misc
Priority: optional
Architecture: all
Maintainer: Contoso Engineering <eng@contoso.com>
Description: Contoso IoT Agent (test package)
 A test package for ADU Gen2 APT handler integration testing.
 This package simulates a real customer agent installation.
Depends: contoso-libs (>= $VERSION)
EOF

# Postinst script
cat > "$PKG_DIR/DEBIAN/postinst" << 'EOF'
#!/bin/bash
echo "contoso-agent: post-install hook executed"
echo "$(date): contoso-agent installed" >> /var/log/contoso-install.log
exit 0
EOF
chmod 755 "$PKG_DIR/DEBIAN/postinst"

# Prerm script
cat > "$PKG_DIR/DEBIAN/prerm" << 'EOF'
#!/bin/bash
echo "contoso-agent: pre-remove hook executed"
exit 0
EOF
chmod 755 "$PKG_DIR/DEBIAN/prerm"

# Binary (dummy)
cat > "$PKG_DIR/usr/bin/contoso-agent" << EOF
#!/bin/bash
echo "Contoso Agent v${VERSION}"
echo "Status: running"
EOF
chmod 755 "$PKG_DIR/usr/bin/contoso-agent"

# Config
cat > "$PKG_DIR/etc/contoso/agent.conf" << EOF
# Contoso Agent Configuration
version=$VERSION
log_level=info
endpoint=https://api.contoso.com
EOF

dpkg-deb --build "$PKG_DIR" "$OUTPUT_DIR/contoso-agent_${VERSION}_all.deb"

# ═══════════════════════════════════════════════════
# Package 2: contoso-libs (dependency)
# ═══════════════════════════════════════════════════
LIB_DIR="$OUTPUT_DIR/contoso-libs_${VERSION}"
mkdir -p "$LIB_DIR/DEBIAN"
mkdir -p "$LIB_DIR/usr/lib/contoso"

cat > "$LIB_DIR/DEBIAN/control" << EOF
Package: contoso-libs
Version: $VERSION
Section: libs
Priority: optional
Architecture: all
Maintainer: Contoso Engineering <eng@contoso.com>
Description: Contoso shared libraries (test package)
 Shared libraries for Contoso IoT suite.
EOF

# Dummy lib
cat > "$LIB_DIR/usr/lib/contoso/libcontoso.so" << EOF
# This is a dummy shared library placeholder for testing
EOF

dpkg-deb --build "$LIB_DIR" "$OUTPUT_DIR/contoso-libs_${VERSION}_all.deb"

# ═══════════════════════════════════════════════════
# Package 3: contoso-agent v2 (for upgrade testing)
# ═══════════════════════════════════════════════════
V2="2.0.0"
PKG_V2_DIR="$OUTPUT_DIR/contoso-agent_${V2}"
mkdir -p "$PKG_V2_DIR/DEBIAN"
mkdir -p "$PKG_V2_DIR/usr/bin"
mkdir -p "$PKG_V2_DIR/etc/contoso"

cat > "$PKG_V2_DIR/DEBIAN/control" << EOF
Package: contoso-agent
Version: $V2
Section: misc
Priority: optional
Architecture: all
Maintainer: Contoso Engineering <eng@contoso.com>
Description: Contoso IoT Agent v2 (test package)
 Updated version for ADU Gen2 APT handler upgrade testing.
Depends: contoso-libs (>= $V2)
EOF

cat > "$PKG_V2_DIR/usr/bin/contoso-agent" << EOF
#!/bin/bash
echo "Contoso Agent v${V2} (upgraded)"
echo "New feature: health reporting"
EOF
chmod 755 "$PKG_V2_DIR/usr/bin/contoso-agent"

cat > "$PKG_V2_DIR/etc/contoso/agent.conf" << EOF
version=$V2
log_level=info
endpoint=https://api.contoso.com
health_check=true
EOF

dpkg-deb --build "$PKG_V2_DIR" "$OUTPUT_DIR/contoso-agent_${V2}_all.deb"

# Also build v2 libs
LIB_V2_DIR="$OUTPUT_DIR/contoso-libs_${V2}"
mkdir -p "$LIB_V2_DIR/DEBIAN"
mkdir -p "$LIB_V2_DIR/usr/lib/contoso"

cat > "$LIB_V2_DIR/DEBIAN/control" << EOF
Package: contoso-libs
Version: $V2
Section: libs
Priority: optional
Architecture: all
Maintainer: Contoso Engineering <eng@contoso.com>
Description: Contoso shared libraries v2 (test package)
EOF

cat > "$LIB_V2_DIR/usr/lib/contoso/libcontoso.so" << EOF
# Dummy v2 library
EOF

dpkg-deb --build "$LIB_V2_DIR" "$OUTPUT_DIR/contoso-libs_${V2}_all.deb"

# Cleanup build dirs
rm -rf "$OUTPUT_DIR/contoso-agent_"*/
rm -rf "$OUTPUT_DIR/contoso-libs_"*/

echo ""
echo "═══════════════════════════════════════════════"
echo "Test packages built successfully!"
echo "═══════════════════════════════════════════════"
echo ""
echo "Packages:"
ls -la "$OUTPUT_DIR"/*.deb
echo ""
echo "To create a local apt repo:"
echo "  cd $OUTPUT_DIR"
echo "  dpkg-scanpackages . /dev/null | gzip -9c > Packages.gz"
echo "  echo 'deb [trusted=yes] file://$OUTPUT_DIR ./' | sudo tee /etc/apt/sources.list.d/contoso-test.list"
echo "  sudo apt-get update"
echo ""
echo "Then test with:"
echo "  sudo apt-get install contoso-libs=$VERSION contoso-agent=$VERSION"
echo "  sudo apt-get install contoso-libs=$V2 contoso-agent=$V2  # upgrade"
