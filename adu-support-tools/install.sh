#!/bin/bash

# Script to install ADU Support Tools via sparse checkout
# Clones only the adu-support-tools folder from azure/iot-hub-device-update repository

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}ADU Support Tools Installer${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Check if already in adu-support-tools directory
if [ -f "${PWD}/VERSION" ] || [ -f "${PWD}/../VERSION" ]; then
    # Check if it's the ADU support tools VERSION file
    if grep -q "^[0-9]\+\.[0-9]\+\.[0-9]\+$" "${PWD}/VERSION" 2>/dev/null || grep -q "^[0-9]\+\.[0-9]\+\.[0-9]\+$" "${PWD}/../VERSION" 2>/dev/null; then
        echo -e "${RED}Error: You are already in an ADU Support Tools directory!${NC}"
        echo -e "${YELLOW}Please run this installer from a different location.${NC}"
        exit 1
    fi
fi

# Repository information
REPO_URL="https://github.com/azure/iot-hub-device-update.git"
BRANCH="adu-support-tools"
SPARSE_PATH="adu-support-tools"

echo -e "${GREEN}Source:${NC} ${REPO_URL}"
echo -e "${GREEN}Branch:${NC} ${BRANCH}"
echo -e "${GREEN}Folder:${NC} ${SPARSE_PATH}"
echo ""

# Check if git is installed
if ! command -v git &> /dev/null; then
    echo -e "${RED}Error: git is not installed.${NC}"
    echo -e "${YELLOW}Please install git first: sudo apt-get install git${NC}"
    exit 1
fi

# Check git version for sparse-checkout support
GIT_VERSION=$(git --version | awk '{print $3}')
echo -e "${BLUE}Git version:${NC} ${GIT_VERSION}"
echo ""

# Check if running interactively
if [ -t 0 ]; then
    # Interactive mode
    INTERACTIVE=true
else
    # Non-interactive mode (piped from curl)
    INTERACTIVE=false
    echo -e "${YELLOW}Note: Running in non-interactive mode${NC}"
    echo -e "${YELLOW}Installing to default location: ./adu-support-tools${NC}"
    echo ""
fi

# Ask for installation directory
if [ "$INTERACTIVE" = true ]; then
    echo -e "${YELLOW}Installation Options:${NC}"
    echo -e "  1. Current directory: ${PWD}"
    echo -e "  2. Specify custom path"
    echo ""
    read -p "Choose option (1/2): " -n 1 -r OPTION
    echo ""
    echo ""
else
    # Default to option 1 in non-interactive mode
    OPTION="1"
fi

if [[ $OPTION == "1" ]]; then
    INSTALL_DIR="${PWD}/adu-support-tools"
    echo -e "${GREEN}Installing to:${NC} ${INSTALL_DIR}"
elif [[ $OPTION == "2" ]]; then
    read -p "Enter target directory path: " -r CUSTOM_PATH
    CUSTOM_PATH="${CUSTOM_PATH/#\~/$HOME}"  # Expand ~
    
    if [[ ! "$CUSTOM_PATH" = /* ]]; then
        # Relative path, make it absolute
        CUSTOM_PATH="${PWD}/${CUSTOM_PATH}"
    fi
    
    INSTALL_DIR="${CUSTOM_PATH}/adu-support-tools"
    echo -e "${GREEN}Installing to:${NC} ${INSTALL_DIR}"
else
    echo -e "${RED}Invalid option. Exiting.${NC}"
    exit 1
fi

echo ""

# Check if directory already exists
if [ -d "$INSTALL_DIR" ]; then
    if [ "$INTERACTIVE" = true ]; then
        echo -e "${YELLOW}Warning: Directory already exists: ${INSTALL_DIR}${NC}"
        read -p "Do you want to remove it and continue? (y/n): " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            echo -e "${YELLOW}Removing existing directory...${NC}"
            rm -rf "$INSTALL_DIR"
        else
            echo -e "${YELLOW}Installation cancelled.${NC}"
            exit 0
        fi
    else
        echo -e "${RED}Error: Directory already exists: ${INSTALL_DIR}${NC}"
        echo -e "${YELLOW}Please remove it first or run the script interactively.${NC}"
        exit 1
    fi
fi

echo ""
# Use temporary directory for git clone
TEMP_CLONE_DIR="/tmp/adu-support-tools-install-$$"

echo -e "${BLUE}Step 1: Cloning repository (sparse checkout)...${NC}"
git clone --filter=blob:none --no-checkout --branch "${BRANCH}" "${REPO_URL}" "${TEMP_CLONE_DIR}"

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Failed to clone repository${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Repository cloned${NC}"

echo ""
echo -e "${BLUE}Step 2: Configuring sparse checkout...${NC}"
cd "${TEMP_CLONE_DIR}"

# Initialize sparse checkout
git sparse-checkout init --cone

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Failed to initialize sparse checkout${NC}"
    rm -rf "${TEMP_CLONE_DIR}"
    exit 1
fi

# Set sparse checkout path
git sparse-checkout set "${SPARSE_PATH}"

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Failed to set sparse checkout path${NC}"
    rm -rf "${TEMP_CLONE_DIR}"
    exit 1
fi
echo -e "${GREEN}✓ Sparse checkout configured${NC}"

echo ""
echo -e "${BLUE}Step 3: Checking out files...${NC}"
git checkout "${BRANCH}"

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Failed to checkout branch${NC}"
    rm -rf "${TEMP_CLONE_DIR}"
    exit 1
fi
echo -e "${GREEN}✓ Files checked out${NC}"

echo ""
echo -e "${BLUE}Step 4: Copying files to target directory...${NC}"
# Create target directory
mkdir -p "${INSTALL_DIR}"

# Copy only the contents of adu-support-tools folder
cp -r "${TEMP_CLONE_DIR}/${SPARSE_PATH}"/* "${INSTALL_DIR}/"

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Failed to copy files${NC}"
    rm -rf "${TEMP_CLONE_DIR}"
    exit 1
fi
echo -e "${GREEN}✓ Files copied${NC}"

echo ""
echo -e "${BLUE}Step 5: Making scripts executable...${NC}"
find "${INSTALL_DIR}" -type f -name "*.sh" -exec chmod +x {} \;
echo -e "${GREEN}✓ Scripts are now executable${NC}"

echo ""
echo -e "${BLUE}Step 6: Cleaning up temporary files...${NC}"
rm -rf "${TEMP_CLONE_DIR}"
echo -e "${GREEN}✓ Cleanup complete${NC}"

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}✓ Installation Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# Display installed version
if [ -f "${INSTALL_DIR}/VERSION" ]; then
    INSTALLED_VERSION=$(cat "${INSTALL_DIR}/VERSION")
    echo -e "${BLUE}Version:${NC} ${INSTALLED_VERSION}"
fi

echo -e "${BLUE}Tools installed at:${NC}"
echo -e "  ${INSTALL_DIR}"
echo ""
echo -e "${BLUE}Available scripts:${NC}"
find "${INSTALL_DIR}" -type f -name "*.sh" ! -name "install.sh" -exec basename {} \; | sed 's/^/  - /'
echo ""
echo -e "${YELLOW}To access the tools:${NC}"
echo -e "  cd ${INSTALL_DIR}"
echo ""
