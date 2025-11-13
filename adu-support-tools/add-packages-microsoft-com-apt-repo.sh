#!/bin/bash

# Script to add packages.microsoft.com APT repository
# Interactively guides user through the setup process

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Microsoft APT Repository Setup${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    echo -e "${RED}Please do not run this script as root.${NC}"
    echo -e "${YELLOW}The script will prompt for sudo when needed.${NC}"
    exit 1
fi

# Detect OS and version
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
    VERSION_ID=$VERSION_ID
    VERSION_CODENAME=$VERSION_CODENAME
else
    echo -e "${RED}Cannot detect OS. /etc/os-release not found.${NC}"
    exit 1
fi

echo -e "${GREEN}Detected OS:${NC} $OS"
echo -e "${GREEN}Version:${NC} $VERSION_ID ($VERSION_CODENAME)"
echo ""

# Validate supported OS
if [[ "$OS" != "ubuntu" && "$OS" != "debian" ]]; then
    echo -e "${RED}Unsupported OS: $OS${NC}"
    echo -e "${YELLOW}This script supports Ubuntu and Debian only.${NC}"
    exit 1
fi

# Construct package URL
PACKAGE_URL="https://packages.microsoft.com/config/${OS}/${VERSION_ID}/packages-microsoft-prod.deb"

echo -e "${BLUE}Configuration:${NC}"
echo -e "  Repository URL: ${PACKAGE_URL}"
echo ""

# Ask for confirmation
read -p "Do you want to proceed with the installation? (y/n): " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo -e "${YELLOW}Installation cancelled.${NC}"
    exit 0
fi

echo ""
echo -e "${BLUE}Step 1: Installing prerequisites...${NC}"
sudo apt-get update
sudo apt-get install -y wget apt-transport-https

echo ""
echo -e "${BLUE}Step 2: Downloading Microsoft repository package...${NC}"
TEMP_DEB="/tmp/packages-microsoft-prod.deb"
if wget -q --show-progress "$PACKAGE_URL" -O "$TEMP_DEB"; then
    echo -e "${GREEN}✓ Download successful${NC}"
else
    echo -e "${RED}✗ Download failed${NC}"
    echo -e "${YELLOW}URL: $PACKAGE_URL${NC}"
    echo -e "${YELLOW}Please check if this version is supported at packages.microsoft.com${NC}"
    exit 1
fi

echo ""
echo -e "${BLUE}Step 3: Installing repository package...${NC}"
sudo dpkg -i "$TEMP_DEB"
echo -e "${GREEN}✓ Repository package installed${NC}"

echo ""
echo -e "${BLUE}Step 4: Cleaning up temporary files...${NC}"
rm -f "$TEMP_DEB"
echo -e "${GREEN}✓ Cleanup complete${NC}"

echo ""
echo -e "${BLUE}Step 5: Updating package lists...${NC}"
sudo apt-get update
echo -e "${GREEN}✓ Package lists updated${NC}"

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}✓ Setup Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo -e "${BLUE}The Microsoft APT repository has been added successfully.${NC}"
echo -e "${BLUE}You can now install Microsoft packages using:${NC}"
echo -e "  ${YELLOW}sudo apt-get install <package-name>${NC}"
echo ""
echo -e "${BLUE}Common packages available:${NC}"
echo -e "  - dotnet-sdk-8.0"
echo -e "  - powershell"
echo -e "  - azure-cli"
echo -e "  - deviceupdate-agent (latest: 1.2.0)"
echo -e "  - deliveryoptimization-agent (latest: 1.1.0)"
echo -e "  - aziot-identity-service (latest: 1.5.6-1)"
echo ""
