#!/bin/bash

# Script to create Azure Device Update (ADU) support bundle
# Collects logs from various ADU client configurations

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}ADU Support Bundle Creator${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Display privacy warning
echo -e "${YELLOW}========================================${NC}"
echo -e "${YELLOW}⚠ PRIVACY AND SECURITY NOTICE${NC}"
echo -e "${YELLOW}========================================${NC}"
echo ""
echo -e "${RED}This script collects system information and logs that may contain:${NC}"
echo -e "  • System configuration details"
echo -e "  • Device identifiers and hostnames"
echo -e "  • Network information"
echo -e "  • Service credentials or tokens"
echo -e "  • File paths and directory structures"
echo -e "  • User-specific data"
echo ""
echo -e "${YELLOW}Before sharing this bundle with Microsoft or others:${NC}"
echo -e "  1. Review the contents of the generated bundle"
echo -e "  2. Remove any sensitive or confidential information"
echo -e "  3. Verify no credentials or secrets are included"
echo ""
echo -e "${BLUE}A README.txt will be generated listing all collected files.${NC}"
echo ""
read -p "Do you understand and agree to proceed? (y/n): " -r
echo ""
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo -e "${YELLOW}Data collection cancelled.${NC}"
    exit 0
fi
echo ""

# Generate bundle filename with timestamp
TIMESTAMP=$(date +%Y%m%d-%H%M%S)
HOSTNAME=$(hostname)
BUNDLE_NAME="adu-support-bundle-${HOSTNAME}-${TIMESTAMP}"

# Get script directory and create output folder
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${SCRIPT_DIR}/adu-support-bundles"
mkdir -p "${OUTPUT_DIR}"

BUNDLE_DIR="/tmp/${BUNDLE_NAME}"
BUNDLE_ARCHIVE="${OUTPUT_DIR}/${BUNDLE_NAME}.tar.gz"

echo -e "${GREEN}Bundle name:${NC} ${BUNDLE_NAME}"
echo -e "${GREEN}Working directory:${NC} ${BUNDLE_DIR}"
echo -e "${GREEN}Output location:${NC} ${OUTPUT_DIR}"
echo ""

# Create temporary directory for bundle
mkdir -p "${BUNDLE_DIR}"

# Define log locations to collect
declare -A LOG_LOCATIONS=(
    ["adu-logs"]="/adu/logs"
    ["var-log-adu"]="/var/log/adu"
    ["do-client-lite-cache"]="/var/cache/do-client-lite/log"
    ["simulator-logs"]="/tmp/aduc-logs"
)

echo -e "${BLUE}Collecting logs from known locations...${NC}"
echo ""

FOUND_COUNT=0

for location_name in "${!LOG_LOCATIONS[@]}"; do
    location_path="${LOG_LOCATIONS[$location_name]}"
    
    if [ -d "$location_path" ]; then
        echo -e "${GREEN}✓ Found:${NC} ${location_path}"
        
        # Create subdirectory in bundle
        dest_dir="${BUNDLE_DIR}/${location_name}"
        mkdir -p "$dest_dir"
        
        # Copy logs (use sudo if permission denied)
        if cp -r "$location_path"/* "$dest_dir/" 2>/dev/null; then
            file_count=$(find "$dest_dir" -type f | wc -l)
            echo -e "  Copied ${file_count} file(s)"
        else
            echo -e "${YELLOW}  Attempting with sudo...${NC}"
            if sudo cp -r "$location_path"/* "$dest_dir/" 2>/dev/null; then
                file_count=$(find "$dest_dir" -type f | wc -l)
                echo -e "  Copied ${file_count} file(s)"
            else
                echo -e "${RED}  Failed to copy logs${NC}"
                rmdir "$dest_dir" 2>/dev/null || true
                continue
            fi
        fi
        
        FOUND_COUNT=$((FOUND_COUNT + 1))
    else
        echo -e "${YELLOW}○ Not found:${NC} ${location_path}"
    fi
done

echo ""

# Collect system information
echo -e "${BLUE}Collecting system information...${NC}"
SYSINFO_DIR="${BUNDLE_DIR}/system-info"
mkdir -p "$SYSINFO_DIR"

# OS information
if [ -f /etc/os-release ]; then
    cp /etc/os-release "$SYSINFO_DIR/" 2>/dev/null || true
    echo -e "${GREEN}✓${NC} OS release info"
fi

# Kernel version
uname -a > "$SYSINFO_DIR/uname.txt" 2>/dev/null || true
echo -e "${GREEN}✓${NC} Kernel version"

# Disk space
df -h > "$SYSINFO_DIR/disk-space.txt" 2>/dev/null || true
echo -e "${GREEN}✓${NC} Disk space"

# Memory info
free -h > "$SYSINFO_DIR/memory.txt" 2>/dev/null || true
echo -e "${GREEN}✓${NC} Memory info"

# Check for ADU service status
if systemctl list-units --full --all | grep -q "deviceupdate-agent"; then
    systemctl status deviceupdate-agent --no-pager > "$SYSINFO_DIR/deviceupdate-agent-status.txt" 2>/dev/null || true
    echo -e "${GREEN}✓${NC} Device Update Agent service status"
fi

if systemctl list-units --full --all | grep -q "deliveryoptimization-agent"; then
    systemctl status deliveryoptimization-agent --no-pager > "$SYSINFO_DIR/deliveryoptimization-agent-status.txt" 2>/dev/null || true
    echo -e "${GREEN}✓${NC} Delivery Optimization Agent service status"
fi

# Check for ADU configuration files
echo ""
echo -e "${BLUE}Collecting configuration files...${NC}"
CONFIG_DIR="${BUNDLE_DIR}/configuration"
mkdir -p "$CONFIG_DIR"

CONFIG_LOCATIONS=(
    "/etc/adu"
    "/etc/deliveryoptimization-agent"
)

for config_path in "${CONFIG_LOCATIONS[@]}"; do
    if [ -d "$config_path" ]; then
        config_name=$(basename "$config_path")
        if sudo cp -r "$config_path" "$CONFIG_DIR/$config_name" 2>/dev/null; then
            echo -e "${GREEN}✓${NC} ${config_path}"
        fi
    fi
done

echo ""

# Generate README.txt with collected files inventory
echo -e "${BLUE}Generating README.txt inventory...${NC}"
README_FILE="${BUNDLE_DIR}/README.txt"

cat > "$README_FILE" << 'EOF'
========================================
ADU SUPPORT BUNDLE README
========================================

IMPORTANT PRIVACY NOTICE:
-------------------------
This bundle contains system logs, configuration files, and diagnostic information
that may include sensitive data such as:
  - System identifiers (hostname, device IDs)
  - Network configuration
  - Service credentials or authentication tokens
  - File paths and directory structures
  - User-specific information

BEFORE SHARING THIS BUNDLE:
---------------------------
1. Review all files in this bundle carefully
2. Remove or redact any sensitive information you do not wish to share
3. Verify no credentials, secrets, or confidential data are present
4. Only share with authorized support personnel

EOF

echo "Bundle Created: $(date)" >> "$README_FILE"
echo "Hostname: ${HOSTNAME}" >> "$README_FILE"
echo "" >> "$README_FILE"
echo "========================================" >> "$README_FILE"
echo "COLLECTED FILES INVENTORY" >> "$README_FILE"
echo "========================================" >> "$README_FILE"
echo "" >> "$README_FILE"

# List all collected files with paths and sizes
find "${BUNDLE_DIR}" -type f ! -name "README.txt" -exec ls -lh {} \; | \
    awk -v bundle_dir="${BUNDLE_DIR}" '{
        size = $5
        path = $NF
        gsub(bundle_dir "/", "", path)
        printf "%-60s %10s\n", path, size
    }' | sort >> "$README_FILE"

# Add summary
echo "" >> "$README_FILE"
echo "========================================" >> "$README_FILE"
echo "SUMMARY" >> "$README_FILE"
echo "========================================" >> "$README_FILE"
TOTAL_FILES=$(find "${BUNDLE_DIR}" -type f ! -name "README.txt" | wc -l)
echo "Total files collected: ${TOTAL_FILES}" >> "$README_FILE"
echo "Log locations found: ${FOUND_COUNT}" >> "$README_FILE"

echo -e "${GREEN}✓${NC} README.txt created with inventory"
echo ""

# Check if we collected anything
if [ $FOUND_COUNT -eq 0 ]; then
    echo -e "${YELLOW}========================================${NC}"
    echo -e "${YELLOW}Warning: No ADU log directories found!${NC}"
    echo -e "${YELLOW}========================================${NC}"
    echo ""
    echo -e "${YELLOW}This could mean:${NC}"
    echo -e "  - ADU is not installed on this system"
    echo -e "  - ADU has not been run yet"
    echo -e "  - Logs are in a custom location"
    echo ""
    read -p "Do you want to create the bundle anyway? (y/n): " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        rm -rf "${BUNDLE_DIR}"
        echo -e "${YELLOW}Bundle creation cancelled.${NC}"
        exit 0
    fi
fi

# Create tarball
echo -e "${BLUE}Creating archive...${NC}"
cd /tmp
tar -czf "${BUNDLE_ARCHIVE}" "${BUNDLE_NAME}" 2>/dev/null

if [ $? -eq 0 ]; then
    BUNDLE_SIZE=$(du -h "${BUNDLE_ARCHIVE}" | cut -f1)
    echo -e "${GREEN}✓ Archive created${NC}"
    
    # Copy README.txt outside of archive
    EXTERNAL_README="${OUTPUT_DIR}/${BUNDLE_NAME}-README.txt"
    cp "${README_FILE}" "${EXTERNAL_README}"
    echo -e "${GREEN}✓ README copied to${NC} ${EXTERNAL_README}"
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}✓ Support Bundle Created Successfully!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo -e "${BLUE}Bundle location:${NC} ${BUNDLE_ARCHIVE}"
    echo -e "${BLUE}Bundle size:${NC} ${BUNDLE_SIZE}"
    echo -e "${BLUE}Log locations found:${NC} ${FOUND_COUNT}"
    echo ""
    echo -e "${RED}⚠ IMPORTANT - REVIEW BEFORE SHARING:${NC}"
    echo -e "  1. Extract and review: tar -xzf ${BUNDLE_ARCHIVE}"
    echo -e "  2. Check README.txt for list of collected files"
    echo -e "  3. Remove any sensitive information from the bundle"
    echo -e "  4. Re-package if needed: cd ${OUTPUT_DIR} && tar -czf ${BUNDLE_NAME}.tar.gz ${BUNDLE_NAME}/"
    echo ""
    echo -e "${YELLOW}Next steps:${NC}"
    echo -e "  1. Review bundle contents thoroughly"
    echo -e "  2. Bundle is ready at: ${BUNDLE_ARCHIVE}"
    echo -e "  3. Share with Microsoft support or your team"
    echo -e "  4. Clean up when done: rm ${BUNDLE_ARCHIVE}"
else
    echo -e "${RED}✗ Failed to create archive${NC}"
    exit 1
fi

# Cleanup temporary directory
rm -rf "${BUNDLE_DIR}"

echo ""
