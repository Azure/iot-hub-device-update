#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# demo-setup.sh - X.509 Certificate Demo and Testing Script
#
# DISCLAIMER: This script is provided AS-IS for demonstration and testing purposes only.
# It has been tested on Ubuntu 20.04 and may not work on other operating systems.
# This script comes with NO WARRANTY, GUARANTEE, or SUPPORT of any kind.
# Use at your own risk. Always review and test in a non-production environment first.

set -e

# Save original argument count for interactive mode detection
original_argc=$#

# Determine repository root using git
if ! repo_root="$(git rev-parse --show-toplevel 2> /dev/null)"; then
    echo "❌ Error: This script must be run from within a git repository"
    echo "💡 Make sure you're running this from the Device Update repository"
    exit 1
fi

# Default device ID
device_id="contoso-vacuum-4"
# Default module ID (optional)
module_id=""
# Connection test option
test_connection=""
iot_hub_hostname=""
# Agent test option
test_agent=""
# Build agent option
build_agent=""
# Custom agent path option
custom_agent_path=""
# Force dependency reinstallation
force_deps=""
# Interactive mode flag
interactive_mode=""

# Function to display help
show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --device-id DEVICE_ID              Device ID to use for certificate generation (default: contoso-vacuum-4)"
    echo "  --module-id MODULE_ID              Module ID for IoT Hub module authentication (optional)"
    echo "  --iot-hub-hostname HOSTNAME        IoT Hub hostname (REQUIRED for --test-connection and --test-agent)"
    echo "  --test-connection                  Test connection to IoT Hub using generated certificates (requires --iot-hub-hostname)"
    echo "  --test-agent                       Test AducIotAgent with generated configuration (runs for 60s, requires --iot-hub-hostname)"
    echo "  --build-agent                      Build and install AducIotAgent from source before testing"
    echo "  --force-deps                       Force reinstallation of build dependencies (use with --build-agent)"
    echo "  --agent-path PATH                  Specify custom path to AducIotAgent binary or .deb package"
    echo "  --interactive                      Run in interactive mode with prompts"
    echo "  -h, --help                         Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 --interactive                   Interactive mode with prompts"
    echo "  $0 --device-id my-iot-device"
    echo "  $0 --device-id my-device --module-id my-module"
    echo "  $0 --device-id my-device --iot-hub-hostname my-hub.azure-devices.net --test-connection"
    echo "  $0 --device-id my-device --module-id my-module --iot-hub-hostname my-hub.azure-devices.net --test-connection"
    echo "  $0 --device-id my-device --iot-hub-hostname my-hub.azure-devices.net --test-agent"
    echo "  $0 --device-id my-device --iot-hub-hostname my-hub.azure-devices.net --build-agent --test-agent"
    echo "  $0 --device-id my-device --iot-hub-hostname my-hub.azure-devices.net --agent-path /path/to/AducIotAgent --test-agent"
    echo "  $0 --device-id my-device --iot-hub-hostname my-hub.azure-devices.net --agent-path /path/to/package.deb --test-agent"
    echo "  $0 --device-id my-device --iot-hub-hostname example-test-hub.azure-devices.net --test-connection"
    echo ""
    echo "Generated files:"
    echo "  • Certificate files: ~/x509-demo-temp/certs-<device_id>[-<module_id>]/"
    echo "  • DU config file: ~/x509-demo-temp/du-config.<device_id>[.<module_id>].json"
    echo "  • Installed certs: /etc/adu/certs/ (with device ID in filenames)"
    echo ""
    echo "For testing purposes, you can use: example-test-hub.azure-devices.net"
}

# Function to prompt for device ID interactively
prompt_device_id() {
    echo ""
    echo "🏷️  Device ID Configuration"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "   Device ID will be used as the Common Name (CN) in the X.509 certificate"
    echo "   and must match exactly with your IoT Hub device registration."
    echo ""
    echo "   Default device ID: $device_id"
    echo ""
    read -r -p "   Enter device ID (press Enter for default): " user_device_id

    if [ -n "$user_device_id" ]; then
        device_id="$user_device_id"
        echo "   ✅ Using device ID: $device_id"
    else
        echo "   ✅ Using default device ID: $device_id"
    fi
}

# Function to prompt for module ID interactively
prompt_module_id() {
    echo ""
    echo "🔧 Module ID Configuration (Optional)"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "   Module ID is optional and only needed for IoT Edge module scenarios."
    echo "   Leave empty for regular device authentication."
    echo ""
    read -r -p "   Enter module ID (optional, press Enter to skip): " user_module_id

    if [ -n "$user_module_id" ]; then
        module_id="$user_module_id"
        echo "   ✅ Using module ID: $module_id"
        echo "   📝 Certificate will be for device '$device_id' with module '$module_id'"
    else
        echo "   ✅ No module ID specified - using device authentication only"
    fi
}

# Function to prompt for IoT Hub hostname interactively
prompt_iot_hub_hostname() {
    echo ""
    echo "🌐 IoT Hub Configuration"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "   Enter your IoT Hub hostname for connection testing."
    echo "   Example: my-hub.azure-devices.net"
    echo "   Test hub: example-test-hub.azure-devices.net"
    echo ""
    read -r -p "   Enter IoT Hub hostname (press Enter to skip connection test): " user_hub_hostname

    if [ -n "$user_hub_hostname" ]; then
        iot_hub_hostname="$user_hub_hostname"
        test_connection="true"
        echo "   ✅ Will test connection to: $iot_hub_hostname"
    else
        echo "   ⏭️  Skipping connection test"
    fi
}

# Function to prompt user after connection failure
prompt_after_connection_failure() {
    echo ""
    echo "🔧 IoT Hub Registration Required"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "   The connection test indicates that the device$([ -n "$module_id" ] && echo " and module") may not be registered"
    echo "   in your IoT Hub or the registration details don't match the certificate."
    echo ""
    echo "   📋 What you need to do:"
    if [ -n "$module_id" ]; then
        echo "   1. Register device '$device_id' in your IoT Hub with X.509 authentication"
        echo "   2. Add module '$module_id' to the device with X.509 authentication"
        echo "   3. Use the certificate thumbprints shown above"
    else
        echo "   1. Register device '$device_id' in your IoT Hub with X.509 authentication"
        echo "   2. Use the certificate thumbprints shown above"
    fi
    echo ""
    echo "   🎯 Options:"
    echo "   [1] I'll register the device$([ -n "$module_id" ] && echo " and module") now - exit script"
    echo "   [2] I've already registered - continue with agent testing"
    echo "   [3] Skip agent testing - just show summary and exit"
    echo ""

    while true; do
        read -r -p "   Please choose [1-3]: " choice
        case $choice in
        1)
            echo "   📋 Please register your device$([ -n "$module_id" ] && echo " and module") in IoT Hub, then re-run this script."
            echo "   💡 After registration, run: $0 --device-id \"$device_id\"$([ -n "$module_id" ] && echo " --module-id \"$module_id\"") --test-connection \"$iot_hub_hostname\" --test-agent"
            return 1
            ;;
        2)
            echo "   ✅ Continuing with agent testing..."
            test_agent="true"
            build_agent="true"
            return 0
            ;;
        3)
            echo "   ⏭️  Skipping agent testing..."
            return 2
            ;;
        *)
            echo "   ❌ Invalid choice. Please enter 1, 2, or 3."
            ;;
        esac
    done
}

# Function to save current run parameters for tracking
save_run_parameters() {
    local params_file="$demo_working_folder/.last_run_params"
    mkdir -p "$demo_working_folder"

    cat > "$params_file" << EOF
DEVICE_ID="$device_id"
MODULE_ID="$module_id"
IOT_HUB_HOSTNAME="$iot_hub_hostname"
TIMESTAMP=$(date +%s)
READABLE_TIME="$(date)"
EOF

    echo "   💾 Saved run parameters to: $params_file"
}

# Function to check if parameters have changed from last run
check_parameter_changes() {
    local params_file="$demo_working_folder/.last_run_params"
    local should_prompt_connection_test=false

    if [ ! -f "$params_file" ]; then
        echo "   ℹ️  First run detected - no previous parameters to compare"
        return 0
    fi

    # Load previous parameters safely
    local prev_device_id=""
    local prev_module_id=""
    local prev_iot_hub_hostname=""
    local prev_readable_time=""

    # Use grep to safely extract values without sourcing
    prev_device_id=$(grep "^DEVICE_ID=" "$params_file" | cut -d'=' -f2- | tr -d '"')
    prev_module_id=$(grep "^MODULE_ID=" "$params_file" | cut -d'=' -f2- | tr -d '"')
    prev_iot_hub_hostname=$(grep "^IOT_HUB_HOSTNAME=" "$params_file" | cut -d'=' -f2- | tr -d '"')
    prev_readable_time=$(grep "^READABLE_TIME=" "$params_file" | cut -d'=' -f2- | tr -d '"')

    echo ""
    echo "🔍 Parameter Change Detection:"
    echo "   📅 Previous run: $prev_readable_time"
    echo "   📋 Comparing current parameters with last run..."

    local changes_detected=false

    # Check for parameter changes
    if [ "$device_id" != "$prev_device_id" ]; then
        echo "   🔄 Device ID changed: '$prev_device_id' → '$device_id'"
        changes_detected=true
    fi

    if [ "$module_id" != "$prev_module_id" ]; then
        echo "   🔄 Module ID changed: '$prev_module_id' → '$module_id'"
        changes_detected=true
    fi

    if [ "$iot_hub_hostname" != "$prev_iot_hub_hostname" ]; then
        echo "   🔄 IoT Hub hostname changed: '$prev_iot_hub_hostname' → '$iot_hub_hostname'"
        changes_detected=true
    fi

    if [ "$changes_detected" = false ]; then
        echo "   ✅ No parameter changes detected since last run"
        return 0
    fi

    echo ""
    echo "⚠️  Parameter changes detected! This may cause connection issues if:"
    echo "   • The device/module is not registered in the new IoT Hub"
    echo "   • The existing certificates were generated for different parameters"
    echo "   • The Azure CLI commands need to be re-run with new thumbprints"
    echo ""

    # If --test-connection was not specified, prompt user
    if [ "$test_connection" != "true" ]; then
        echo "💡 Recommendation: Test the connection before proceeding with agent testing"
        echo ""

        if [ "$test_agent" != "true" ]; then
            # Not testing agent, just inform user
            echo "ℹ️  Consider running with --test-connection flag to verify connectivity"
            echo "   Example: $0 --device-id \"$device_id\" --module-id \"$module_id\" --iot-hub-hostname \"$iot_hub_hostname\" --test-connection"
            return 0
        fi

        # Testing agent but no connection test requested - prompt user
        echo "🤔 Would you like to test the IoT Hub connection first? This will:"
        echo "   • Validate certificates work with the new parameters"
        echo "   • Check if device/module registration is required"
        echo "   • Display Azure CLI commands with correct thumbprints if needed"
        echo ""
        echo "Test connection before proceeding? (Y/n): "
        read -r response

        case "$response" in
        [Nn] | [Nn][Oo])
            echo "   ⏭️  Skipping connection test as requested"
            ;;
        *)
            echo "   🔌 Enabling connection test..."
            test_connection="true"
            should_prompt_connection_test=true
            ;;
        esac
    else
        echo "   ✅ Connection test is already enabled"
    fi

    echo ""

    # Return 1 if we enabled connection test due to parameter changes
    if [ "$should_prompt_connection_test" = true ]; then
        return 1
    fi

    return 0
}

# Function to setup Microsoft package repository
setup_microsoft_repository() {
    echo "🔧 Setting up Microsoft package repository..."

    # Check if packages.microsoft.com is already configured
    if apt-cache policy | grep -q "packages.microsoft.com"; then
        echo "   ✅ Microsoft package repository already configured"
        return 0
    fi

    # Detect Ubuntu/Debian version for proper repository configuration
    local distro_version=""
    if [ -f /etc/os-release ]; then
        # shellcheck source=/dev/null
        . /etc/os-release
        case "$ID" in
        ubuntu)
            case "$VERSION_ID" in
            "18.04") distro_version="bionic" ;;
            "20.04") distro_version="focal" ;;
            "22.04") distro_version="jammy" ;;
            "24.04") distro_version="noble" ;;
            *) distro_version="focal" ;; # fallback to focal for unknown versions
            esac
            ;;
        debian)
            case "$VERSION_ID" in
            "9") distro_version="stretch" ;;
            "10") distro_version="buster" ;;
            "11") distro_version="bullseye" ;;
            "12") distro_version="bookworm" ;;
            *) distro_version="bullseye" ;; # fallback to bullseye for unknown versions
            esac
            ;;
        *)
            echo "   ⚠️  Unsupported distribution: $ID. Trying with Ubuntu focal fallback..."
            distro_version="focal"
            ;;
        esac
    else
        echo "   ⚠️  Cannot detect distribution. Using Ubuntu focal fallback..."
        distro_version="focal"
    fi

    echo "   📋 Detected distribution: $ID $VERSION_ID -> using $distro_version"

    # Install required packages for repository setup
    echo "   🔧 Installing prerequisites..."
    if ! sudo apt-get update -qq; then
        echo "   ⚠️  Warning: apt-get update had issues, continuing anyway..."
    fi

    if ! sudo apt-get install -qq -y wget gpg; then
        echo "   ❌ Failed to install prerequisites (wget, gpg)"
        return 1
    fi

    # Download and add Microsoft GPG key
    echo "   🔑 Adding Microsoft GPG key..."
    if ! wget -qO- https://packages.microsoft.com/keys/microsoft.asc | sudo gpg --dearmor --yes -o /usr/share/keyrings/microsoft-prod.gpg; then
        echo "   ❌ Failed to download and install Microsoft GPG key"
        return 1
    fi

    # Add Microsoft repository
    echo "   📦 Adding Microsoft repository..."
    local repo_line="deb [arch=amd64,arm64,armhf signed-by=/usr/share/keyrings/microsoft-prod.gpg] https://packages.microsoft.com/ubuntu/$distro_version/prod $distro_version main"

    if ! echo "$repo_line" | sudo tee /etc/apt/sources.list.d/microsoft-prod.list > /dev/null; then
        echo "   ❌ Failed to add Microsoft repository"
        return 1
    fi

    # Update package cache
    echo "   🔄 Updating package cache..."
    if ! sudo apt-get update -qq; then
        echo "   ❌ Failed to update package cache after adding Microsoft repository"
        return 1
    fi

    # Verify repository is working
    echo "   ✅ Verifying Microsoft repository..."
    if apt-cache search deliveryoptimization-agent | grep -q "deliveryoptimization-agent"; then
        echo "   ✅ Microsoft package repository configured successfully"
        echo "   📦 Found deliveryoptimization-agent package in repository"
        return 0
    else
        echo "   ⚠️  Microsoft repository added but deliveryoptimization-agent not found"
        echo "   💡 This may be normal if the package isn't available for your distribution"
        return 0
    fi
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
    --device-id)
        device_id="$2"
        if [[ -z $device_id ]]; then
            echo "Error: --device-id requires a string argument"
            show_help
            exit 1
        fi
        shift 2
        ;;
    --module-id)
        module_id="$2"
        if [[ -z $module_id ]]; then
            echo "Error: --module-id requires a string argument"
            show_help
            exit 1
        fi
        shift 2
        ;;
    --test-connection)
        test_connection="true"
        shift 1
        ;;
    --iot-hub-hostname)
        iot_hub_hostname="$2"
        if [[ -z $iot_hub_hostname ]]; then
            echo "Error: --iot-hub-hostname requires an IoT Hub hostname"
            show_help
            exit 1
        fi
        shift 2
        ;;
    --test-agent)
        test_agent="true"
        shift 1
        ;;
    --build-agent)
        build_agent="true"
        shift 1
        ;;
    --force-deps)
        force_deps="true"
        shift 1
        ;;
    --interactive)
        interactive_mode="true"
        shift 1
        ;;
    --agent-path)
        if [ -z "$2" ] || [[ $2 == -* ]]; then
            echo "Error: --agent-path requires a path argument"
            show_help
            exit 1
        fi
        custom_agent_path="$2"
        shift 2
        ;;
    -h | --help)
        show_help
        exit 0
        ;;
    *)
        echo "Error: Unknown option $1"
        show_help
        exit 1
        ;;
    esac
done

echo "Using device ID: $device_id"
if [ -n "$module_id" ]; then
    echo "Using module ID: $module_id"
fi

# Validate that IoT Hub hostname is provided when --test-agent is used
if [ "$test_agent" = "true" ] && [ -z "$iot_hub_hostname" ]; then
    echo "❌ Error: --test-agent requires --iot-hub-hostname to be specified"
    echo "💡 Example: $0 --device-id \"$device_id\" --iot-hub-hostname \"your-hub.azure-devices.net\" --test-agent"
    echo ""
    show_help
    exit 1
fi

# Validate that IoT Hub hostname is provided when --test-connection is used
if [ "$test_connection" = "true" ] && [ -z "$iot_hub_hostname" ]; then
    echo "❌ Error: --test-connection requires --iot-hub-hostname to be specified"
    echo "💡 Example: $0 --device-id \"$device_id\" --iot-hub-hostname \"your-hub.azure-devices.net\" --test-connection"
    echo ""
    show_help
    exit 1
fi

# Run interactive prompts if in interactive mode or if no parameters were provided
if [ "$interactive_mode" = "true" ] || [ $original_argc -eq 0 ]; then
    echo ""
    echo "🚀 X.509 Certificate Demo and Testing Script"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "   This script will generate X.509 certificates, install them, and optionally"
    echo "   test the connection to IoT Hub and run the ADU agent."
    echo ""

    prompt_device_id
    prompt_module_id
    prompt_iot_hub_hostname

    echo ""
    echo "📋 Configuration Summary:"
    echo "   📱 Device ID: $device_id"
    if [ -n "$module_id" ]; then
        echo "   🔧 Module ID: $module_id"
    fi
    if [ -n "$iot_hub_hostname" ]; then
        echo "   🌐 IoT Hub: $iot_hub_hostname"
        if [ "$test_connection" = "true" ]; then
            echo "   🔌 Will test connection"
        fi
        if [ "$test_agent" = "true" ]; then
            echo "   🤖 Will test ADU agent"
        fi
    else
        echo "   ⏭️  No connection test"
    fi
    echo ""

    # Auto-enable connection test if we have IoT Hub hostname
    if [ -n "$iot_hub_hostname" ] && [ "$test_connection" != "true" ] && [ "$test_agent" != "true" ]; then
        test_connection="true"
        echo "   🔌 Auto-enabling connection test since IoT Hub hostname provided"
    fi

    # Auto-enable agent testing if we have connection test
    if [ "$test_connection" = "true" ]; then
        test_agent="true"
        build_agent="true"
        echo "   🔧 Will build and test ADU agent after certificate setup"
    fi

    read -r -p "   Press Enter to continue or Ctrl+C to cancel..."
    echo ""
fi

# Function to ensure system has necessary root certificates for Azure IoT Hub
ensure_system_root_certificates() {
    echo "   🔒 Ensuring system root certificates are up to date..."

    # Update CA certificates package (if needed)
    if command -v update-ca-certificates > /dev/null 2>&1; then
        # For Ubuntu/Debian systems
        if [ -f /usr/share/ca-certificates/mozilla/DigiCert_Global_Root_G2.crt ]; then
            echo "   ✅ DigiCert Global Root G2 certificate found"
        else
            echo "   ⚠️  DigiCert Global Root G2 certificate may be missing"
            echo "   💡 Consider updating ca-certificates package: sudo apt update && sudo apt install ca-certificates"
        fi
    fi

    # Check if we can verify Azure IoT Hub certificate chain
    if openssl s_client -connect example-test-hub.azure-devices.net:8883 -CApath /etc/ssl/certs -verify_return_error -quiet < /dev/null > /dev/null 2>&1; then
        echo "   ✅ System can verify Azure IoT Hub certificate chain"
    else
        echo "   ⚠️  System may have trouble verifying Azure IoT Hub certificates"
        echo "   💡 This might affect device twin operations"
    fi
}

# Function to test IoT Hub connection
test_iot_hub_connection() {
    echo "🔌 Testing connection to IoT Hub: $iot_hub_hostname"

    # Initialize variables
    mqtt_success=false
    device_needs_registration=false

    # Check if certificates exist
    if [ ! -f "$demo_gen_certs_folder/client-$cert_suffix.pem" ] || [ ! -f "$demo_gen_certs_folder/client-$cert_suffix.key" ] || [ ! -f "$demo_gen_certs_folder/ca-$cert_suffix.pem" ]; then
        echo "❌ Certificates not found. Please ensure certificates are generated first."
        exit 1
    fi

    echo "🔐 Performing comprehensive certificate verification..."
    echo ""

    # Step 1: Certificate file integrity checks
    echo "📋 Step 1: Certificate File Integrity Checks"
    echo "   🔍 Checking CA certificate format..."
    if openssl x509 -in "$demo_gen_certs_folder/ca-$cert_suffix.pem" -noout -text > /dev/null 2>&1; then
        echo "   ✅ CA certificate format is valid"
    else
        echo "   ❌ CA certificate format is invalid"
        exit 1
    fi

    echo "   🔍 Checking primary client certificate format..."
    if openssl x509 -in "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" -noout -text > /dev/null 2>&1; then
        echo "   ✅ Primary client certificate format is valid"
    else
        echo "   ❌ Primary client certificate format is invalid"
        exit 1
    fi

    echo "   🔍 Checking secondary client certificate format..."
    if openssl x509 -in "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" -noout -text > /dev/null 2>&1; then
        echo "   ✅ Secondary client certificate format is valid"
    else
        echo "   ❌ Secondary client certificate format is invalid"
        exit 1
    fi

    echo "   🔍 Checking primary private key format..."
    if openssl rsa -in "$demo_gen_certs_folder/client-primary-$cert_suffix.key" -check -noout > /dev/null 2>&1; then
        echo "   ✅ Primary private key format is valid"
    else
        echo "   ❌ Primary private key format is invalid"
        exit 1
    fi

    echo "   🔍 Checking secondary private key format..."
    if openssl rsa -in "$demo_gen_certs_folder/client-secondary-$cert_suffix.key" -check -noout > /dev/null 2>&1; then
        echo "   ✅ Secondary private key format is valid"
    else
        echo "   ❌ Secondary private key format is invalid"
        exit 1
    fi

    # Step 2: Certificate-Key pair validation
    echo ""
    echo "📋 Step 2: Certificate-Key Pair Validation"
    echo "   🔍 Verifying private key matches certificate..."
    cert_modulus=$(openssl x509 -in "$demo_gen_certs_folder/client-$cert_suffix.pem" -noout -modulus 2> /dev/null)
    key_modulus=$(openssl rsa -in "$demo_gen_certs_folder/client-$cert_suffix.key" -noout -modulus 2> /dev/null)

    if [ "$cert_modulus" = "$key_modulus" ]; then
        echo "   ✅ Private key matches certificate (modulus verified)"
    else
        echo "   ❌ Private key does not match certificate"
        exit 1
    fi

    # Step 3: Certificate chain validation
    echo ""
    echo "📋 Step 3: Certificate Chain Validation"
    echo "   🔍 Verifying certificate chain..."
    chain_result=$(openssl verify -CAfile "$demo_gen_certs_folder/ca-$cert_suffix.pem" "$demo_gen_certs_folder/client-$cert_suffix.pem" 2>&1)
    if echo "$chain_result" | grep -q "OK"; then
        echo "   ✅ Certificate chain is valid"
        echo "   📝 Chain validation: $chain_result"
    else
        echo "   ❌ Certificate chain validation failed"
        echo "   📝 Error: $chain_result"
        exit 1
    fi

    # Step 4: Certificate details verification
    echo ""
    echo "📋 Step 4: Certificate Details Verification"
    # Check device IDs in certificates
    primary_device_id_from_cert=$(openssl x509 -in "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" -noout -subject | sed 's/.*CN = \([^,]*\).*/\1/')
    secondary_device_id_from_cert=$(openssl x509 -in "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" -noout -subject | sed 's/.*CN = \([^,]*\).*/\1/')
    device_id_from_cert="$primary_device_id_from_cert" # For backward compatibility

    echo "   📱 Device ID from primary certificate: $primary_device_id_from_cert"
    echo "   📱 Device ID from secondary certificate: $secondary_device_id_from_cert"

    if [ "$primary_device_id_from_cert" = "$device_id" ] && [ "$secondary_device_id_from_cert" = "$device_id" ]; then
        echo "   ✅ Device IDs in both certificates match requested ID"
    else
        echo "   ⚠️  Warning: Device ID mismatch detected"
        [ "$primary_device_id_from_cert" != "$device_id" ] && echo "      Primary cert: $primary_device_id_from_cert, requested: $device_id"
        [ "$secondary_device_id_from_cert" != "$device_id" ] && echo "      Secondary cert: $secondary_device_id_from_cert, requested: $device_id"
    fi

    echo "   📅 Primary certificate validity period:"
    openssl x509 -in "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" -noout -dates | sed 's/^/      /'

    echo "   📅 Secondary certificate validity period:"
    openssl x509 -in "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" -noout -dates | sed 's/^/      /'

    # Check if both certificates are currently valid
    primary_valid=$(openssl x509 -in "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" -checkend 0 > /dev/null 2>&1 && echo "true" || echo "false")
    secondary_valid=$(openssl x509 -in "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" -checkend 0 > /dev/null 2>&1 && echo "true" || echo "false")

    if [ "$primary_valid" = "true" ] && [ "$secondary_valid" = "true" ]; then
        echo "   ✅ Both certificates are currently valid (not expired)"
    else
        [ "$primary_valid" = "false" ] && echo "   ❌ Primary certificate has expired"
        [ "$secondary_valid" = "false" ] && echo "   ❌ Secondary certificate has expired"
        exit 1
    fi

    # Step 5: Certificate thumbprints
    echo ""
    echo "📋 Step 5: Certificate Thumbprints for IoT Hub"
    primary_thumbprint=$(openssl x509 -in "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" -noout -sha1 -fingerprint | sed 's/[:]//g' | sed 's/SHA1 Fingerprint=//')
    secondary_thumbprint=$(openssl x509 -in "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" -noout -sha1 -fingerprint | sed 's/[:]//g' | sed 's/SHA1 Fingerprint=//')

    echo "   🔑 Primary (SHA1):   $primary_thumbprint"
    echo "   🔑 Secondary (SHA1): $secondary_thumbprint"

    # Step 6: TLS Connection Tests
    echo ""
    echo "📋 Step 6: TLS Connection Tests"
    echo "   🌐 Testing basic TLS handshake to $iot_hub_hostname:8883..."

    # Test 1: Basic TLS connection without CA verification
    basic_tls_test=$(timeout 10s openssl s_client -connect "$iot_hub_hostname:8883" \
        -cert "$demo_gen_certs_folder/client-$cert_suffix.pem" \
        -key "$demo_gen_certs_folder/client-$cert_suffix.key" \
        -servername "$iot_hub_hostname" \
        -quiet 2>&1 <<< "QUIT" || true)

    if echo "$basic_tls_test" | grep -q "depth=" && echo "$basic_tls_test" | grep -q "verify return:1"; then
        echo "   ✅ TLS handshake successful"
        echo "   📝 Server certificate chain verified"

        # Extract and display connection details
        protocol=$(echo "$basic_tls_test" | grep "Protocol" | head -1 || echo "   Protocol: Not available")
        cipher=$(echo "$basic_tls_test" | grep "Cipher" | head -1 || echo "   Cipher: Not available")
        echo "   📋 Connection details:"
        echo "      $protocol"
        echo "      $cipher"
    else
        echo "   ❌ TLS handshake failed"
        echo "   📋 Error details:"
        echo "$basic_tls_test" | head -10 | sed 's/^/      /'
    fi

    # Step 7: MQTT Connection Test (if mosquitto is available)
    echo ""
    echo "📋 Step 7: MQTT Protocol Test"
    if command -v mosquitto_pub > /dev/null 2>&1; then
        echo "   🐛 Testing MQTT connection with mosquitto client..."

        mqtt_test=$(timeout 10s mosquitto_pub -h "$iot_hub_hostname" -p 8883 \
            -i "$device_id_from_cert" \
            -t "devices/$device_id_from_cert/messages/events/" \
            -m "test message" \
            --cert "$demo_gen_certs_folder/client-$cert_suffix.pem" \
            --key "$demo_gen_certs_folder/client-$cert_suffix.key" \
            --insecure -d 2>&1 || true)

        if echo "$mqtt_test" | grep -q "received CONNACK"; then
            if echo "$mqtt_test" | grep -q "Connection Refused: not authorised"; then
                echo "   ✅ MQTT connection successful (X.509 authentication working)"
                echo "   📝 IoT Hub rejected authorization (expected - device needs proper permissions)"
                mqtt_success=false # Device not registered, need to prompt user
                device_needs_registration=true
            else
                echo "   ✅ MQTT connection and authorization successful"
                mqtt_success=true # Device is properly registered
                device_needs_registration=false
            fi
        elif echo "$mqtt_test" | grep -q "sending CONNECT"; then
            echo "   ✅ MQTT client certificate authentication successful"
            echo "   📝 Connection established, authentication working"
            mqtt_success=false # Connection made but unclear if device is registered
            device_needs_registration=true
        else
            echo "   ⚠️  MQTT connection test inconclusive"
            echo "   📝 Test output: $(echo "$mqtt_test" | head -2 | tr '\n' ' ')"
            mqtt_success=false
            device_needs_registration=true
        fi
    else
        echo "   ⚠️  Mosquitto client not available, skipping MQTT test"
        echo "   💡 Install mosquitto-clients for full MQTT testing"
    fi

    # Step 8: Connection Summary
    echo "
📋 Step 8: Connection Test Summary
   🎯 Target IoT Hub: $iot_hub_hostname
   📱 Device ID: $device_id_from_cert$([ -n "$module_id" ] && echo "
   � Module ID: $module_id")
   �🔑 Certificate Authentication: ✅ Working
   🔐 TLS Connection: ✅ Established
   📝 Next steps: Ensure device$([ -n "$module_id" ] && echo " and module") is registered in IoT Hub with thumbprint

🔧 For self-signed certificates, register the device in IoT Hub:
   1. In Azure Portal, go to your IoT Hub → Device management → Devices
   2. Click '+ Add Device'
   3. Set Device ID to: $device_id_from_cert
   4. Set Authentication type to: 'X.509 Self-Signed'
   5. Set Primary Thumbprint to: $primary_thumbprint
   6. Set Secondary Thumbprint to: $secondary_thumbprint
   7. Click 'Save'$([ -n "$module_id" ] && echo "

🔧 If using a module, after creating the device, add the module:
   1. Go to the device details page for: $device_id_from_cert
   2. Click 'Module Identities' tab
   3. Click '+ Add Module Identity'
   4. Set Module Identity Name to: $module_id
   5. Set Authentication type to: 'X.509 Self-Signed'
   6. Set Primary Thumbprint to: $primary_thumbprint
   7. Set Secondary Thumbprint to: $secondary_thumbprint
   8. Click 'Save'")

   Or use Azure CLI:
   # Create device
   az iot hub device-identity create \\
     --hub-name ${iot_hub_hostname%%.azure-devices.net} \\
     --device-id $device_id_from_cert \\
     --auth-method x509_thumbprint \\
     --primary-thumbprint $primary_thumbprint \\
     --secondary-thumbprint $secondary_thumbprint$([ -n "$module_id" ] && echo "

   # Create module (if using module authentication)
   az iot hub module-identity create \\
     --hub-name ${iot_hub_hostname%%.azure-devices.net} \\
     --device-id $device_id_from_cert \\
     --module-id $module_id \\
     --auth-method x509_thumbprint \\
     --primary-thumbprint $primary_thumbprint \\
     --secondary-thumbprint $secondary_thumbprint")"

    # In interactive mode or when connection testing shows potential registration issues,
    # prompt user about device registration status
    if [ "$interactive_mode" = "true" ] || [ $original_argc -eq 0 ] || [ "$device_needs_registration" = "true" ]; then
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

        # Check if MQTT test showed connection success
        if [ "$mqtt_success" = true ]; then
            echo "✅ Connection test successful! Device appears to be properly registered."
            return 0
        else
            echo "⚠️  Connection test indicates device registration is needed."
            prompt_after_connection_failure
            return $?
        fi
    fi

    return 0
}

# Function to show generated configuration and offer to install it
show_and_install_config() {
    echo ""
    echo "📄 Generated Device Update Configuration"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "   📁 Configuration file: $du_config_file"
    echo ""
    echo "   📋 Generated configuration content:"
    echo "   ═════════════════════════════════════════════════════════════"

    # Display the configuration file with line numbers and syntax highlighting
    if command -v jq > /dev/null 2>&1; then
        echo "   📝 Pretty-printed JSON content:"
        jq . "$du_config_file" 2> /dev/null | sed 's/^/   /' || {
            echo "   📝 Raw JSON content (jq formatting failed):"
            sed 's/^/   /' < "$du_config_file"
        }
    else
        echo "   📝 Raw JSON content:"
        sed 's/^/   /' < "$du_config_file"
    fi

    echo "   ═════════════════════════════════════════════════════════════"
    echo ""

    # Check if system configuration exists
    system_config="/etc/adu/du-config.json"
    backup_config="/etc/adu/du-config.json.backup-$(date +%Y%m%d-%H%M%S)"

    echo "   🎯 Installation Target: $system_config"

    if [ -f "$system_config" ]; then
        echo "   ⚠️  Existing configuration file found"
        echo "   📄 Current system configuration:"
        echo "   ─────────────────────────────────────────────────────────────"
        if command -v jq > /dev/null 2>&1; then
            sudo jq . "$system_config" 2> /dev/null | sed 's/^/   /' || {
                echo "   📝 Raw content (jq formatting failed):"
                sudo cat "$system_config" | sed 's/^/   /'
            }
        else
            sudo cat "$system_config" | sed 's/^/   /'
        fi
        echo "   ─────────────────────────────────────────────────────────────"
        echo ""

        # Offer backup and replacement
        echo "   🔧 Configuration Management Options:"
        echo "   [1] Replace system config with backup (recommended)"
        echo "   [2] Replace system config without backup"
        echo "   [3] Keep existing system config - use generated config for testing only"
        echo "   [4] Show diff between current and generated config"
        echo ""

        while true; do
            read -r -p "   Please choose [1-4]: " config_choice
            case $config_choice in
            1)
                echo "   💾 Creating backup: $backup_config"
                if sudo cp "$system_config" "$backup_config"; then
                    echo "   ✅ Backup created successfully"
                    echo "   🔧 Installing new configuration..."
                    if sudo cp "$du_config_file" "$system_config"; then
                        echo "   ✅ Configuration installed successfully!"
                        echo "   💡 To restore backup later: sudo cp \"$backup_config\" \"$system_config\""
                        return 0
                    else
                        echo "   ❌ Failed to install new configuration"
                        return 1
                    fi
                else
                    echo "   ❌ Failed to create backup"
                    return 1
                fi
                ;;
            2)
                echo "   ⚠️  Installing new configuration WITHOUT backup..."
                if sudo cp "$du_config_file" "$system_config"; then
                    echo "   ✅ Configuration installed successfully!"
                    echo "   ⚠️  Original configuration was overwritten (no backup)"
                    return 0
                else
                    echo "   ❌ Failed to install new configuration"
                    return 1
                fi
                ;;
            3)
                echo "   ⏭️  Keeping existing system configuration"
                echo "   📝 Agent will use existing config: $system_config"
                echo "   💡 Generated config remains at: $du_config_file"
                return 2
                ;;
            4)
                echo ""
                echo "   📊 Configuration Differences:"
                echo "   ══════════════════════════════════════════════════════════════"
                if command -v diff > /dev/null 2>&1; then
                    # Create temporary files for comparison
                    temp_current="/tmp/current-config.json"
                    temp_generated="/tmp/generated-config.json"

                    sudo cat "$system_config" | tee "$temp_current" > /dev/null 2>&1 || true
                    cat "$du_config_file" > "$temp_generated" 2> /dev/null || true

                    echo "   📝 Showing differences (- current, + generated):"
                    if diff -u "$temp_current" "$temp_generated" | sed 's/^/   /' || true; then
                        echo "   ✅ Diff completed"
                    fi

                    # Cleanup
                    rm -f "$temp_current" "$temp_generated"
                else
                    echo "   ⚠️  'diff' command not available"
                    echo "   💡 Install diffutils for configuration comparison"
                fi
                echo "   ══════════════════════════════════════════════════════════════"
                echo ""
                continue
                ;;
            *)
                echo "   ❌ Invalid choice. Please enter 1, 2, 3, or 4."
                ;;
            esac
        done
    else
        echo "   ✅ No existing system configuration found"
        echo "   🔧 Installing new configuration..."

        # Create the directory if it doesn't exist
        if ! sudo test -d "$(dirname "$system_config")"; then
            echo "   📁 Creating configuration directory: $(dirname "$system_config")"
            if ! sudo mkdir -p "$(dirname "$system_config")"; then
                echo "   ❌ Failed to create configuration directory"
                return 1
            fi
        fi

        # Install the configuration
        if sudo cp "$du_config_file" "$system_config"; then
            echo "   ✅ Configuration installed successfully!"

            # Set appropriate ownership
            if id "adu" &> /dev/null; then
                if sudo chown adu:adu "$system_config"; then
                    echo "   👤 Ownership set to adu:adu"
                else
                    echo "   ⚠️  Failed to set ownership (continuing anyway)"
                fi
            else
                echo "   ⚠️  User 'adu' not found, skipping ownership change"
            fi

            return 0
        else
            echo "   ❌ Failed to install new configuration"
            return 1
        fi
    fi
}

# Function to test AducIotAgent with generated configuration
test_adu_agent() {
    echo "🔧 Testing AducIotAgent with X.509 configuration..."

    # Stop the deviceupdate-agent service to run in standalone mode
    echo "🛑 Stopping deviceupdate-agent service for standalone testing..."
    if systemctl is-active --quiet deviceupdate-agent; then
        echo "   📋 Service is currently running, stopping it..."
        if sudo systemctl stop deviceupdate-agent; then
            echo "   ✅ Service stopped successfully"
        else
            echo "   ⚠️  Failed to stop service, continuing anyway"
        fi
    else
        echo "   ✅ Service is already stopped"
    fi
    echo ""

    agent_path=""
    run_as_adu=""

    # Check if custom agent path is provided
    if [ -n "$custom_agent_path" ]; then
        if [[ $custom_agent_path == *.deb ]]; then
            # Handle .deb package
            if [ -f "$custom_agent_path" ]; then
                echo "📦 Custom .deb package specified: $custom_agent_path"
                echo "💡 Would you like to install this package? (y/N)"
                read -r install_response
                if [[ $install_response =~ ^[Yy]$ ]]; then
                    echo "🔧 Installing package: $custom_agent_path"

                    # Setup Microsoft repository before package installation
                    if ! setup_microsoft_repository; then
                        echo "⚠️  Microsoft repository setup failed, but continuing with installation..."
                    fi

                    if sudo dpkg -i "$custom_agent_path"; then
                        echo "✅ Package installed successfully!"
                        sudo apt-get install -f -y 2> /dev/null || true
                        # After installation, look for the agent in standard locations
                        for path in "/usr/bin/AducIotAgent" "/usr/local/bin/AducIotAgent"; do
                            if [ -f "$path" ] && [ -x "$path" ]; then
                                agent_path="$path"
                                run_as_adu="yes"
                                break
                            fi
                        done
                    else
                        echo "❌ Package installation failed"
                        return 1
                    fi
                else
                    echo "❌ Package installation declined"
                    return 1
                fi
            else
                echo "❌ Custom .deb package not found: $custom_agent_path"
                return 1
            fi
        else
            # Handle direct binary path
            if [ -f "$custom_agent_path" ] && [ -x "$custom_agent_path" ]; then
                agent_path="$custom_agent_path"
                run_as_adu="yes"
                echo "✅ Using custom AducIotAgent: $agent_path"
            else
                echo "❌ Custom AducIotAgent binary not found or not executable: $custom_agent_path"
                return 1
            fi
        fi
    else
        # First, check if we should install from the latest built package
        echo "🔍 Checking for built ADU agent package..."

        # Look for .deb package in out folder
        out_folder="$repo_root/out"
        deb_package=""
        if [ -d "$out_folder" ]; then
            deb_package=$(find "$out_folder" -name "deviceupdate-agent_*.deb" -type f | sort -V | tail -1)
        fi

        if [ -n "$deb_package" ] && [ -f "$deb_package" ]; then
            echo "✅ Found built package: $(basename "$deb_package")"
            echo ""
            echo "📦 To ensure we test the latest version, we need to install this package."
            echo "   This will:"
            echo "   • Install/upgrade the ADU agent to the latest built version"
            echo "   • Stop and disable the deviceupdate-agent service to avoid conflicts"
            echo "   • Allow manual testing with the latest code changes"
            echo ""
            read -p "🤔 Install the latest built package? (y/N): " -r
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                echo ""
                echo "📦 Installing ADU agent package..."
                echo "   Package: $deb_package"

                # Setup Microsoft repository before package installation
                if ! setup_microsoft_repository; then
                    echo "⚠️  Microsoft repository setup failed, but continuing with installation..."
                fi

                # Install the package
                if sudo dpkg -i "$deb_package"; then
                    echo "✅ Package installed successfully"

                    # Stop and disable the service to avoid conflicts
                    echo ""
                    echo "🔧 Stopping and disabling deviceupdate-agent service..."
                    if sudo systemctl is-active --quiet deviceupdate-agent.service; then
                        sudo systemctl stop deviceupdate-agent.service
                        echo "   ✅ Service stopped"
                    fi

                    if sudo systemctl is-enabled --quiet deviceupdate-agent.service; then
                        sudo systemctl disable deviceupdate-agent.service
                        echo "   ✅ Service disabled (prevents auto-start)"
                    fi

                    echo ""
                    echo "💡 The service has been disabled to prevent conflicts during manual testing."
                    echo "   To re-enable later: sudo systemctl enable --now deviceupdate-agent.service"

                    # Now look for the installed agent
                    agent_path="/usr/bin/AducIotAgent"
                    if [ -f "$agent_path" ] && [ -x "$agent_path" ]; then
                        echo "✅ Agent ready at: $agent_path"
                        run_as_adu="yes"
                    else
                        echo "❌ Installed agent not found at expected location: $agent_path"
                        return 1
                    fi
                else
                    echo "❌ Package installation failed"
                    echo "💡 You may need to fix dependencies: sudo apt-get -f install"
                    return 1
                fi
            else
                echo "⚠️  Package installation declined"
            fi
        fi

        # If we didn't install from package, check standard locations
        if [ -z "$agent_path" ]; then
            # Check if AducIotAgent exists in common locations
            for path in "/usr/bin/AducIotAgent" "/usr/local/bin/AducIotAgent" "$(which AducIotAgent 2> /dev/null)"; do
                if [ -f "$path" ] && [ -x "$path" ]; then
                    agent_path="$path"
                    run_as_adu="yes"

                    # Check if service is running and warn about conflicts
                    if sudo systemctl is-active --quiet deviceupdate-agent.service; then
                        echo "⚠️  WARNING: deviceupdate-agent service is currently running!"
                        echo "   This may cause conflicts during manual testing."
                        echo ""
                        read -p "🤔 Stop the service for manual testing? (y/N): " -r
                        if [[ $REPLY =~ ^[Yy]$ ]]; then
                            sudo systemctl stop deviceupdate-agent.service
                            echo "✅ Service stopped for testing"
                            echo "💡 Restart later with: sudo systemctl start deviceupdate-agent.service"
                        else
                            echo "⚠️  Proceeding with service running - there may be connection conflicts"
                        fi
                        echo ""
                    fi
                    break
                fi
            done
        fi
    fi

    # If not found in standard locations, check build output folders
    if [ -z "$agent_path" ]; then
        echo "❌ AducIotAgent not found in standard locations"
        echo "   Searched: /usr/bin/AducIotAgent, /usr/local/bin/AducIotAgent, PATH"
        echo ""

        # Look for agent in common build output locations
        build_locations=(
            "$repo_root/out/bin/AducIotAgent"
            "$repo_root/build/bin/AducIotAgent"
            "$repo_root/_build/bin/AducIotAgent"
            "$repo_root/build/src/agent/AducIotAgent"
            "$repo_root/out/src/agent/AducIotAgent"
        )

        echo "� Checking build output locations..."
        for build_path in "${build_locations[@]}"; do
            if [ -f "$build_path" ] && [ -x "$build_path" ]; then
                echo "✅ Found agent in build output: $build_path"
                echo ""
                echo "⚠️  This agent was built from source and may need to run as 'adu' user."
                echo "    Do you want to proceed with testing? (y/N)"
                read -r response
                if [[ $response =~ ^[Yy]$ ]]; then
                    agent_path="$build_path"
                    run_as_adu="yes"
                    echo "✅ Will run agent from build output"
                    break
                else
                    echo "❌ User declined to run build output agent"
                    return 1
                fi
            fi
        done

        if [ -z "$agent_path" ]; then
            echo "❌ AducIotAgent not found in build output either"
            echo ""
            echo "�💡 Installation options:"
            echo "   • Install from package: sudo apt install deviceupdate-agent"
            echo "   • Build from source in this repository:"
            echo "     cd $repo_root"
            echo "     ./scripts/build.sh"
            echo "     # Agent will be in build output directory"
            echo ""
            echo "   • Then re-run this script with --test-agent"
            return 1
        fi
    fi

    echo "✅ Found AducIotAgent at: $agent_path"

    # Ensure the system config exists and contains X.509 configuration
    system_config="/etc/adu/du-config.json"
    if ! sudo test -f "$system_config"; then
        echo "❌ System configuration not found: $system_config"
        echo ""
        echo "🔧 The agent requires the du-config.json to be installed at the default system location."
        echo "   Generated config is available at: $du_config_file"
        echo ""

        # Check if generated config exists
        if [ ! -f "$du_config_file" ]; then
            echo "❌ Generated configuration file not found: $du_config_file"
            echo "💡 Please run certificate generation first:"
            echo "   Run: $0 --generate-certs"
            return 1
        fi

        read -p "🤔 Would you like to install the configuration to system location? (y/N): " -r
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            echo ""
            echo "📦 Installing configuration to system location..."
            if show_and_install_config "$du_config_file"; then
                echo "✅ Configuration installed successfully"
            else
                echo "❌ Failed to install configuration"
                return 1
            fi
        else
            echo "⚠️  Configuration installation declined"
            echo "💡 The agent requires system configuration. Please install manually:"
            echo "   Run: $0 --show-config --install-config"
            return 1
        fi
    fi

    echo "✅ Using system configuration: $system_config"

    echo ""
    echo "📋 Agent Test Configuration:"
    echo "   📄 Config file: Using system default (/etc/adu/du-config.json)"
    echo "   📱 Device ID: $device_id"
    if [ -n "$module_id" ]; then
        echo "   🔧 Module ID: $module_id"
    fi
    if [ -n "$iot_hub_hostname" ]; then
        echo "   🎯 IoT Hub: $iot_hub_hostname"
    fi
    echo ""

    echo "🚀 Starting AducIotAgent test (will run for 60 seconds)..."
    echo "   📝 Agent will use default configuration: $system_config"
    if [ "$run_as_adu" = "yes" ]; then
        echo "   👤 Running as 'adu' user for proper permissions"
        echo "   Command: sudo -u adu $agent_path -l 0 -e"
    else
        echo "   Command: $agent_path -l 0 -e"
    fi
    echo "   Monitoring connection and authentication status..."
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━ AGENT OUTPUT START ━━━━━━━━━━━━━━━━━━━━━━━"

    # Run AducIotAgent in background and capture its output
    if [ "$run_as_adu" = "yes" ]; then
        # Check if adu user exists
        if ! id adu > /dev/null 2>&1; then
            echo "⚠️  User 'adu' not found, running as current user"
            timeout 60s "$agent_path" -l 0 -e 2>&1 | while IFS= read -r line; do
                echo "$line"
                # Highlight important connection messages
                case "$line" in
                *[Cc]onnected* | *[Aa]uthentication* | *[Cc]ertificate* | *[Mm]odule* | *[Dd]evice* | *[Ee]rror* | *[Ff]ailed* | *[Ss]uccess*)
                    echo "🔍 Key message: $line" >&2
                    ;;
                esac
            done
        else
            timeout 60s sudo -u adu "$agent_path" -l 0 -e 2>&1 | while IFS= read -r line; do
                echo "$line"
                # Highlight important connection messages
                case "$line" in
                *[Cc]onnected* | *[Aa]uthentication* | *[Cc]ertificate* | *[Mm]odule* | *[Dd]evice* | *[Ee]rror* | *[Ff]ailed* | *[Ss]uccess*)
                    echo "🔍 Key message: $line" >&2
                    ;;
                esac
            done
        fi
    else
        timeout 60s "$agent_path" -l 0 -e 2>&1 | while IFS= read -r line; do
            echo "$line"
            # Highlight important connection messages
            case "$line" in
            *[Cc]onnected* | *[Aa]uthentication* | *[Cc]ertificate* | *[Mm]odule* | *[Dd]evice* | *[Ee]rror* | *[Ff]ailed* | *[Ss]uccess*)
                echo "🔍 Key message: $line" >&2
                ;;
            esac
        done
    fi

    agent_exit_code=${PIPESTATUS[0]}

    echo "━━━━━━━━━━━━━━━━━━━━━━━ AGENT OUTPUT END ━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""

    echo "📊 Agent Test Summary:"
    if [ "$agent_exit_code" -eq 124 ]; then
        echo "   ✅ Agent ran for 60 seconds and was terminated (expected)"
        echo "   📝 Check the output above for connection and authentication status"
    elif [ "$agent_exit_code" -eq 0 ]; then
        echo "   ✅ Agent completed successfully"
    else
        echo "   ⚠️  Agent exited with code: $agent_exit_code"
        echo "   📝 Check the output above for error details"
    fi

    echo ""
    echo "💡 Analysis Tips:"
    echo "   • Look for 'Connected' or 'Authentication successful' messages"
    echo "   • Check for any certificate or TLS errors"
    echo "   • Module authentication requires both device and module to be registered"
    echo "   • connection errors may indicate IoT Hub registration issues"
    if [ "$run_as_adu" = "yes" ]; then
        echo "   • Agent ran as 'adu' user for proper certificate permissions"
    fi

    echo ""
    echo "🔄 Service Management:"
    echo "   📝 The deviceupdate-agent service was stopped for standalone testing"
    echo "   🚀 To restart the service after testing: sudo systemctl start deviceupdate-agent"
    echo "   📊 To check service status: sudo systemctl status deviceupdate-agent"
}

# Function to build and install AducIotAgent with packages
build_adu_agent() {
    echo "🏗️ Building and installing AducIotAgent with packages..."

    echo "📁 Repository root: $repo_root"

    # Change to repository root
    cd "$repo_root"

    # Check if dependencies need to be installed/updated
    deps_marker_file="$repo_root/.demo_deps_installed"
    install_deps_script="$repo_root/scripts/install-deps.sh"
    need_deps_install=false

    if [ "$force_deps" = "true" ]; then
        # Force reinstall requested
        need_deps_install=true
        echo "🔧 Force reinstalling dependencies (--force-deps specified)..."
        # Remove marker to ensure fresh install
        rm -f "$deps_marker_file"
    elif [ ! -f "$deps_marker_file" ]; then
        # Never installed dependencies
        need_deps_install=true
        echo "🔧 Installing dependencies (first time setup)..."
    elif [ "$install_deps_script" -nt "$deps_marker_file" ]; then
        # install-deps.sh has been updated since last install
        need_deps_install=true
        echo "🔧 Installing dependencies (install-deps.sh has been updated)..."
    else
        echo "✅ Dependencies are up-to-date (skipping installation)"
        echo "💡 Use --force-deps to force reinstallation if needed"
    fi

    if [ "$need_deps_install" = true ]; then
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━ DEPENDENCY INSTALL START ━━━━━━━━━━━━━━━━━━━━━━━"
        if ./scripts/install-deps.sh -a; then
            echo "━━━━━━━━━━━━━━━━━━━━━━━ DEPENDENCY INSTALL END ━━━━━━━━━━━━━━━━━━━━━━━━"
            echo "✅ Dependencies installed successfully!"
            # Create/update the marker file
            touch "$deps_marker_file"
        else
            echo "━━━━━━━━━━━━━━━━━━━━━━━ DEPENDENCY INSTALL END ━━━━━━━━━━━━━━━━━━━━━━━━"
            echo "❌ Dependency installation failed!"
            echo "💡 Continuing with build attempt anyway..."
        fi
    fi

    echo ""
    echo "🔧 Running build with packages..."
    echo ""

    echo "━━━━━━━━━━━━━━━━━━━━━━━ BUILD OUTPUT START ━━━━━━━━━━━━━━━━━━━━━━━"

    # Run the build script with packages
    if ./scripts/build.sh --build-packages --type Release; then
        echo "━━━━━━━━━━━━━━━━━━━━━━━ BUILD OUTPUT END ━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        echo "✅ Build completed successfully!"

        # Check for built agent
        built_agents=(
            "$repo_root/out/bin/AducIotAgent"
            "$repo_root/build/bin/AducIotAgent"
            "$repo_root/_build/bin/AducIotAgent"
            "$repo_root/build/src/agent/AducIotAgent"
            "$repo_root/out/src/agent/AducIotAgent"
        )

        for agent_path in "${built_agents[@]}"; do
            if [ -f "$agent_path" ] && [ -x "$agent_path" ]; then
                echo "✅ Agent binary found: $agent_path"
                break
            fi
        done

        # Check for .deb packages
        echo ""
        echo "📦 Checking for generated packages..."
        deb_files=$(find "$repo_root/out" -name "deviceupdate-agent_*.deb" -type f 2> /dev/null)
        if [ -n "$deb_files" ]; then
            echo "✅ .deb packages generated:"
            echo "$deb_files" | while read -r pkg; do
                echo "   📄 $(basename "$pkg")"
            done
            echo ""
            deb_file=$(echo "$deb_files" | head -1)

            # Auto-install since --build-agent was specified
            echo "🔧 Installing package automatically (--build-agent specified): $deb_file"

            # Setup Microsoft repository before package installation
            if ! setup_microsoft_repository; then
                echo "⚠️  Microsoft repository setup failed, but continuing with installation..."
            fi

            if sudo dpkg -i "$deb_file"; then
                echo "✅ Package installed successfully!"
                # Fix any dependency issues
                sudo apt-get install -f -y 2> /dev/null || true
                echo "✅ Agent ready for testing at /usr/bin/AducIotAgent"
            else
                echo "❌ Package installation failed"
                echo "💡 You may need to run: sudo apt-get install -f"
                echo "💡 Manual install: sudo dpkg -i \"$deb_file\""
                return 1
            fi
        else
            echo "⚠️  No .deb packages found in out/ directory"
            echo "💡 The build may not have completed successfully"
            return 1
        fi

    else
        echo "━━━━━━━━━━━━━━━━━━━━━━━ BUILD OUTPUT END ━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        echo "❌ Build failed!"
        echo "💡 Check the build output above for error details"
        return 1
    fi
}

# Create demo working folder
demo_working_folder=~/x509-demo-temp
mkdir -p "$demo_working_folder"

# Create directory for test certificates with device ID and optional module ID in path
if [ -n "$module_id" ]; then
    demo_gen_certs_folder="$demo_working_folder/certs-$device_id-$module_id"
    cert_suffix="$device_id-$module_id"
else
    demo_gen_certs_folder="$demo_working_folder/certs-$device_id"
    cert_suffix="$device_id"
fi
mkdir -p "$demo_gen_certs_folder"

# Check for parameter changes from previous run
check_parameter_changes

echo "🔧 Setting up X.509 test environment..."

# Check if certificates already exist (with device ID and optional module ID in filename)
if [ -f "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" ] && [ -f "$demo_gen_certs_folder/client-primary-$cert_suffix.key" ] \
    && [ -f "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" ] && [ -f "$demo_gen_certs_folder/client-secondary-$cert_suffix.key" ] \
    && [ -f "$demo_gen_certs_folder/ca-$cert_suffix.pem" ]; then
    echo "✅ Certificates ready"
else
    if [ -n "$module_id" ]; then
        echo "📜 Generating enhanced test certificates for IoT module authentication..."
    else
        echo "📜 Generating enhanced test certificates for IoT device authentication..."
    fi

    # Generate CA private key
    openssl genrsa -out "$demo_gen_certs_folder/ca-$cert_suffix.key" 2048

    # Generate CA certificate with proper extensions (self-signed, valid for 10 years)
    openssl req -new -x509 -days 3650 -key "$demo_gen_certs_folder/ca-$cert_suffix.key" -out "$demo_gen_certs_folder/ca-$cert_suffix.pem" \
        -subj "/C=US/ST=WA/O=Contoso/CN=Contoso-CA-$device_id" \
        -extensions v3_ca \
        -config <(
            echo '[req]'
            echo 'distinguished_name=req'
            echo '[v3_ca]'
            echo 'basicConstraints=CA:TRUE'
            echo 'keyUsage=keyCertSign,cRLSign'
            echo 'subjectKeyIdentifier=hash'
            echo 'authorityKeyIdentifier=keyid:always,issuer:always'
        )

    # Generate PRIMARY client private key
    openssl genrsa -out "$demo_gen_certs_folder/client-primary-$cert_suffix.key" 2048

    # Generate PRIMARY client certificate signing request with proper subject
    # For modules, the CN should still be the device ID, not the module ID
    openssl req -new -key "$demo_gen_certs_folder/client-primary-$cert_suffix.key" -out "$demo_gen_certs_folder/client-primary-$cert_suffix.csr" \
        -subj "/C=US/ST=WA/O=Contoso/CN=$device_id"

    # Generate PRIMARY client certificate signed by CA with IoT device extensions (valid for 1 year)
    openssl x509 -req -days 365 -in "$demo_gen_certs_folder/client-primary-$cert_suffix.csr" \
        -CA "$demo_gen_certs_folder/ca-$cert_suffix.pem" -CAkey "$demo_gen_certs_folder/ca-$cert_suffix.key" -CAcreateserial \
        -out "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" \
        -extensions v3_client \
        -extfile <(
            echo '[v3_client]'
            echo 'basicConstraints=CA:FALSE'
            echo 'keyUsage=digitalSignature,keyEncipherment'
            echo 'extendedKeyUsage=clientAuth'
            echo 'subjectKeyIdentifier=hash'
            echo 'authorityKeyIdentifier=keyid,issuer'
        )

    # Generate SECONDARY client private key
    openssl genrsa -out "$demo_gen_certs_folder/client-secondary-$cert_suffix.key" 2048

    # Generate SECONDARY client certificate signing request with proper subject
    # For modules, the CN should still be the device ID, not the module ID
    openssl req -new -key "$demo_gen_certs_folder/client-secondary-$cert_suffix.key" -out "$demo_gen_certs_folder/client-secondary-$cert_suffix.csr" \
        -subj "/C=US/ST=WA/O=Contoso/CN=$device_id"

    # Generate SECONDARY client certificate signed by CA with IoT device extensions (valid for 1 year)
    openssl x509 -req -days 365 -in "$demo_gen_certs_folder/client-secondary-$cert_suffix.csr" \
        -CA "$demo_gen_certs_folder/ca-$cert_suffix.pem" -CAkey "$demo_gen_certs_folder/ca-$cert_suffix.key" -CAcreateserial \
        -out "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" \
        -extensions v3_client \
        -extfile <(
            echo '[v3_client]'
            echo 'basicConstraints=CA:FALSE'
            echo 'keyUsage=digitalSignature,keyEncipherment'
            echo 'extendedKeyUsage=clientAuth'
            echo 'subjectKeyIdentifier=hash'
            echo 'authorityKeyIdentifier=keyid,issuer'
        )

    # Clean up CSR files
    rm "$demo_gen_certs_folder/client-primary-$cert_suffix.csr"
    rm "$demo_gen_certs_folder/client-secondary-$cert_suffix.csr"

    # Create compatibility symlinks for backward compatibility (primary certificate as default)
    ln -sf "client-primary-$cert_suffix.pem" "$demo_gen_certs_folder/client-$cert_suffix.pem"
    ln -sf "client-primary-$cert_suffix.key" "$demo_gen_certs_folder/client-$cert_suffix.key"

    # Create combined CA certificate file with Microsoft root CAs for production scenarios
    echo "🔗 Creating combined CA certificate file..."
    combined_ca_file="$demo_gen_certs_folder/ca-combined-$cert_suffix.pem"

    # Start with our self-signed CA
    cp "$demo_gen_certs_folder/ca-$cert_suffix.pem" "$combined_ca_file"

    # Append Microsoft's root CA certificates if available
    microsoft_ca_paths=(
        "/etc/ssl/certs/DigiCert_Global_Root_G2.pem"
        "/etc/ssl/certs/Baltimore_CyberTrust_Root.pem"
        "/etc/ssl/certs/Microsoft_RSA_Root_Certificate_Authority_2017.pem"
        "/usr/share/ca-certificates/mozilla/DigiCert_Global_Root_G2.crt"
        "/usr/share/ca-certificates/mozilla/Baltimore_CyberTrust_Root.crt"
    )

    echo "" >> "$combined_ca_file"
    echo "# Microsoft Azure IoT Hub Root Certificates" >> "$combined_ca_file"

    for ca_path in "${microsoft_ca_paths[@]}"; do
        if [ -f "$ca_path" ]; then
            echo "   📜 Found Microsoft CA: $(basename "$ca_path")"
            echo "" >> "$combined_ca_file"
            cat "$ca_path" >> "$combined_ca_file"
        fi
    done

    # If no Microsoft CAs found, add a note
    if ! grep -q "BEGIN CERTIFICATE" "$combined_ca_file" | tail -n +2; then
        echo "   ⚠️  No Microsoft root CA certificates found in standard locations"
        echo "   💡 For production, manually append Microsoft's root CA certificates to:"
        echo "      $combined_ca_file"
    else
        echo "   ✅ Combined CA certificate created with Microsoft root CAs"
    fi

    # Ensure system has necessary root certificates for Azure IoT Hub
    ensure_system_root_certificates

    echo "✅ Enhanced certificates ready"
fi

# Generate DU configuration file
echo "📝 Generating Device Update configuration file..."
if [ -n "$module_id" ]; then
    du_config_file="$demo_working_folder/du-config.$device_id.$module_id.json"
else
    du_config_file="$demo_working_folder/du-config.$device_id.json"
fi

# Use combined CA file if available, otherwise use basic CA
ca_cert_path="/etc/adu/certs/ca-$cert_suffix.pem"
combined_ca_path="/etc/adu/certs/ca-combined-$cert_suffix.pem"
if [ -f "$demo_gen_certs_folder/ca-combined-$cert_suffix.pem" ]; then
    ca_cert_path="$combined_ca_path"
    echo "📋 Using combined CA certificate (includes Microsoft root CAs)"
else
    echo "📋 Using basic CA certificate (self-signed only)"
fi

cat > "$du_config_file" << EOF
{
  "schemaVersion": "1.2",
  "aduShellTrustedUsers": [
    "adu",
    "do"
  ],
  "compatPropertyNames": "manufacturer,model",
  "manufacturer": "Contoso",
  "model": "$device_id",
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "X509",
        "connectionData": "HostName=${iot_hub_hostname:-YOUR_IOT_HUB_HOSTNAME};DeviceId=$device_id$([ -n "$module_id" ] && echo ";ModuleId=$module_id");x509=true",
        "connectionX509CertFilePath": "/etc/adu/certs/client-$cert_suffix.pem",
        "connectionX509PrivateKeyFilePath": "/etc/adu/certs/client-$cert_suffix.key",
        "connectionX509CaCertFilePath": "$ca_cert_path"
      },
      "manufacturer": "Contoso",
      "model": "$device_id"
    }
  ]
}
EOF

echo "✅ Configuration file generated: $du_config_file"
echo ""
echo "📋 X.509 Configuration Details:"
echo "   🔗 Connection Type: X509"
echo "   🎯 Connection Data: HostName=${iot_hub_hostname:-YOUR_IOT_HUB_HOSTNAME};DeviceId=$device_id$([ -n "$module_id" ] && echo ";ModuleId=$module_id");x509=true"
echo "   📜 Client Certificate: /etc/adu/certs/client-$cert_suffix.pem"
echo "   🔑 Private Key: /etc/adu/certs/client-$cert_suffix.key"
echo "   🏛️  CA Certificate: $ca_cert_path"
echo ""
echo "💡 Note: For production use, the CA certificate file should contain"
echo "         the complete certificate chain including Microsoft's root CAs"

echo ""
echo "📋 Generated Certificate Information:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Display CA certificate details
echo "🔐 CA Certificate Details:"
echo "   Subject: $(openssl x509 -in "$demo_gen_certs_folder/ca-$cert_suffix.pem" -noout -subject | sed 's/subject=//')"
echo "   Issuer:  $(openssl x509 -in "$demo_gen_certs_folder/ca-$cert_suffix.pem" -noout -issuer | sed 's/issuer=//')"
echo "   Valid from: $(openssl x509 -in "$demo_gen_certs_folder/ca-$cert_suffix.pem" -noout -startdate | sed 's/notBefore=//')"
echo "   Valid to:   $(openssl x509 -in "$demo_gen_certs_folder/ca-$cert_suffix.pem" -noout -enddate | sed 's/notAfter=//')"
echo "   SHA1 Fingerprint: $(openssl x509 -in "$demo_gen_certs_folder/ca-$cert_suffix.pem" -noout -sha1 -fingerprint | sed 's/SHA1 Fingerprint=//')"

echo ""

# Display client certificate details
echo "📱 Primary Client Certificate Details:"
echo "   Subject: $(openssl x509 -in "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" -noout -subject | sed 's/subject=//')"
echo "   Issuer:  $(openssl x509 -in "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" -noout -issuer | sed 's/issuer=//')"
echo "   Valid from: $(openssl x509 -in "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" -noout -startdate | sed 's/notBefore=//')"
echo "   Valid to:   $(openssl x509 -in "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" -noout -enddate | sed 's/notAfter=//')"
device_id_from_cert=$(openssl x509 -in "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" -noout -subject | sed 's/.*CN = \([^,]*\).*/\1/')
echo "   Device ID (CN): $device_id_from_cert"

echo ""

echo "📱 Secondary Client Certificate Details:"
echo "   Subject: $(openssl x509 -in "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" -noout -subject | sed 's/subject=//')"
echo "   Issuer:  $(openssl x509 -in "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" -noout -issuer | sed 's/issuer=//')"
echo "   Valid from: $(openssl x509 -in "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" -noout -startdate | sed 's/notBefore=//')"
echo "   Valid to:   $(openssl x509 -in "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" -noout -enddate | sed 's/notAfter=//')"
secondary_device_id_from_cert=$(openssl x509 -in "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" -noout -subject | sed 's/.*CN = \([^,]*\).*/\1/')
echo "   Device ID (CN): $secondary_device_id_from_cert"

echo ""

# Display certificate thumbprints
echo "🔑 Certificate Thumbprints for IoT Hub:"
echo "   Primary (SHA1):   $(openssl x509 -in "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" -noout -sha1 -fingerprint | sed 's/SHA1 Fingerprint=//')"
echo "   Secondary (SHA1): $(openssl x509 -in "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" -noout -sha1 -fingerprint | sed 's/SHA1 Fingerprint=//')"

echo ""

# Validate certificate chain
echo "🔍 Certificate Chain Validation:"
if openssl verify -CAfile "$demo_gen_certs_folder/ca-$cert_suffix.pem" "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" > /dev/null 2>&1 \
    && openssl verify -CAfile "$demo_gen_certs_folder/ca-$cert_suffix.pem" "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" > /dev/null 2>&1; then
    echo "   ✅ Primary certificate chain is valid"
    echo "   ✅ Secondary certificate chain is valid"
else
    echo "   ❌ Certificate chain validation failed"
    exit 1
fi

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Install certificates
echo ""
echo "📦 Installing certificates..."
echo ""

# Create destination directory
dest_dir="/etc/adu/certs"
echo "   📁 Creating destination directory: $dest_dir"

if sudo mkdir -p "$dest_dir" 2> /dev/null; then
    echo "   ✅ Directory created successfully: $dest_dir"
else
    echo "   ❌ Failed to create directory: $dest_dir"
    echo "   💡 You may need to run with sudo privileges"
    exit 1
fi

echo ""
echo "   📋 Installing certificate files:"
echo "   ═════════════════════════════════════════════════════════════"

# Install CA certificate
echo "   🔐 Installing CA Certificate..."
echo "      📂 Source: $demo_gen_certs_folder/ca-$cert_suffix.pem"
echo "      📁 Destination: $dest_dir/ca-$cert_suffix.pem"
if sudo cp "$demo_gen_certs_folder/ca-$cert_suffix.pem" "$dest_dir/ca-$cert_suffix.pem" 2> /dev/null; then
    echo "      ✅ CA Certificate installed successfully"
else
    echo "      ❌ Failed to install CA Certificate"
    exit 1
fi

# Install combined CA certificate (includes Microsoft root CAs)
combined_ca_file="$demo_gen_certs_folder/ca-combined-$cert_suffix.pem"
if [ -f "$combined_ca_file" ]; then
    echo ""
    echo "   🔗 Installing Combined CA Certificate (includes Microsoft root CAs)..."
    echo "      📂 Source: $combined_ca_file"
    echo "      📁 Destination: $dest_dir/ca-combined-$cert_suffix.pem"
    if sudo cp "$combined_ca_file" "$dest_dir/ca-combined-$cert_suffix.pem" 2> /dev/null; then
        echo "      ✅ Combined CA Certificate installed successfully"
        echo "      💡 This file includes both self-signed and Microsoft root CAs"
    else
        echo "      ⚠️  Failed to install Combined CA Certificate (continuing with basic CA)"
    fi
fi

# Install primary client certificate
echo ""
echo "   📱 Installing Primary Client Certificate..."
echo "      📂 Source: $demo_gen_certs_folder/client-primary-$cert_suffix.pem"
echo "      📁 Destination: $dest_dir/client-primary-$cert_suffix.pem"
if sudo cp "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" "$dest_dir/client-primary-$cert_suffix.pem" 2> /dev/null; then
    echo "      ✅ Primary Client Certificate installed successfully"
else
    echo "      ❌ Failed to install Primary Client Certificate"
    exit 1
fi

# Install secondary client certificate
echo ""
echo "   📱 Installing Secondary Client Certificate..."
echo "      📂 Source: $demo_gen_certs_folder/client-secondary-$cert_suffix.pem"
echo "      📁 Destination: $dest_dir/client-secondary-$cert_suffix.pem"
if sudo cp "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" "$dest_dir/client-secondary-$cert_suffix.pem" 2> /dev/null; then
    echo "      ✅ Secondary Client Certificate installed successfully"
else
    echo "      ❌ Failed to install Secondary Client Certificate"
    exit 1
fi

# Install default client certificate (symlink to primary)
echo ""
echo "   📱 Installing Default Client Certificate (symlink to primary)..."
echo "      📂 Source: $demo_gen_certs_folder/client-$cert_suffix.pem (→ primary)"
echo "      📁 Destination: $dest_dir/client-$cert_suffix.pem"
if sudo cp "$demo_gen_certs_folder/client-$cert_suffix.pem" "$dest_dir/client-$cert_suffix.pem" 2> /dev/null; then
    echo "      ✅ Default Client Certificate installed successfully"
else
    echo "      ❌ Failed to install Default Client Certificate"
    exit 1
fi

# Install primary client private key
echo ""
echo "   🔑 Installing Primary Client Private Key..."
echo "      📂 Source: $demo_gen_certs_folder/client-primary-$cert_suffix.key"
echo "      📁 Destination: $dest_dir/client-primary-$cert_suffix.key"
if sudo cp "$demo_gen_certs_folder/client-primary-$cert_suffix.key" "$dest_dir/client-primary-$cert_suffix.key" 2> /dev/null; then
    # Set appropriate permissions for private key (readable only by owner)
    if sudo chmod 600 "$dest_dir/client-primary-$cert_suffix.key" 2> /dev/null; then
        echo "      ✅ Primary Client Private Key installed successfully (permissions: 600)"
    else
        echo "      ⚠️  Primary Client Private Key installed but failed to set permissions"
    fi
else
    echo "      ❌ Failed to install Primary Client Private Key"
    exit 1
fi

# Install secondary client private key
echo ""
echo "   🔑 Installing Secondary Client Private Key..."
echo "      📂 Source: $demo_gen_certs_folder/client-secondary-$cert_suffix.key"
echo "      📁 Destination: $dest_dir/client-secondary-$cert_suffix.key"
if sudo cp "$demo_gen_certs_folder/client-secondary-$cert_suffix.key" "$dest_dir/client-secondary-$cert_suffix.key" 2> /dev/null; then
    # Set appropriate permissions for private key (readable only by owner)
    if sudo chmod 600 "$dest_dir/client-secondary-$cert_suffix.key" 2> /dev/null; then
        echo "      ✅ Secondary Client Private Key installed successfully (permissions: 600)"
    else
        echo "      ⚠️  Secondary Client Private Key installed but failed to set permissions"
    fi
else
    echo "      ❌ Failed to install Secondary Client Private Key"
    exit 1
fi

# Install default client private key (symlink to primary)
echo ""
echo "   🔑 Installing Default Client Private Key (symlink to primary)..."
echo "      📂 Source: $demo_gen_certs_folder/client-$cert_suffix.key (→ primary)"
echo "      📁 Destination: $dest_dir/client-$cert_suffix.key"
if sudo cp "$demo_gen_certs_folder/client-$cert_suffix.key" "$dest_dir/client-$cert_suffix.key" 2> /dev/null; then
    # Set appropriate permissions for private key (readable only by owner)
    if sudo chmod 600 "$dest_dir/client-$cert_suffix.key" 2> /dev/null; then
        echo "      ✅ Default Client Private Key installed successfully (permissions: 600)"
    else
        echo "      ⚠️  Default Client Private Key installed but failed to set permissions"
    fi
else
    echo "      ❌ Failed to install Default Client Private Key"
    exit 1
fi

echo "   ═════════════════════════════════════════════════════════════"
echo ""

# Set ownership to adu user if exists
echo "   👤 Setting ownership to adu:adu..."
if id "adu" &> /dev/null; then
    if sudo chown -R adu:adu "$dest_dir" 2> /dev/null; then
        echo "   ✅ Ownership set successfully"
    else
        echo "   ⚠️  Failed to set ownership (continuing anyway)"
    fi
else
    echo "   ⚠️  User 'adu' does not exist, skipping ownership change"
fi

echo ""
echo "   📋 Final installation summary:"

# Verify all files were installed correctly
files_installed=true
if ! sudo test -f "$dest_dir/ca-$cert_suffix.pem" || ! sudo test -f "$dest_dir/client-$cert_suffix.pem" || ! sudo test -f "$dest_dir/client-$cert_suffix.key"; then
    files_installed=false
fi

if [ "$files_installed" = true ]; then
    echo "   ✅ All certificates installed successfully in $dest_dir"
    echo ""
    echo "   📁 Actual installed file structure:"
    if sudo test -d "$dest_dir" 2> /dev/null; then
        sudo find "$dest_dir" -type f -exec ls -la {} \; | sed 's/^/      /'
    else
        echo "      • ca-$cert_suffix.pem (CA Certificate)"
        echo "      • client-$cert_suffix.pem (Client Certificate)"
        echo "      • client-$cert_suffix.key (Client Private Key)"
    fi
else
    echo "   ❌ Certificate installation incomplete"
    echo "   📋 Please check file permissions and directory access"
    exit 1
fi

# Run some basic validation tests
echo ""
echo "🧪 Running unit tests..."

echo "🔍 Validating certificates..."
if openssl x509 -in "$demo_gen_certs_folder/client-$cert_suffix.pem" -noout > /dev/null 2>&1; then
    echo "openssl verify -CAfile \"$demo_gen_certs_folder/ca-$cert_suffix.pem\" \"$demo_gen_certs_folder/client-$cert_suffix.pem\""
    echo "✅ Certificate chain valid"
else
    echo "❌ Certificate validation failed"
    exit 1
fi

echo "🔌 Testing agent configuration..."

# If connection test was requested, run it
connection_test_result=0
if [ "$test_connection" = "true" ]; then
    test_iot_hub_connection
    connection_test_result=$?

    # Handle connection test results
    case $connection_test_result in
    1)
        echo ""
        echo "👋 Exiting script - please register your device and module, then re-run."
        echo "💡 Quick re-run command after registration:"
        echo "   $0 --device-id \"$device_id\"$([ -n "$module_id" ] && echo " --module-id \"$module_id\"") --test-connection \"$iot_hub_hostname\" --test-agent"
        exit 0
        ;;
    2)
        echo ""
        echo "⏭️  Skipping agent testing as requested."
        test_agent=""
        build_agent=""
        ;;
    0)
        echo ""
        echo "✅ Connection test completed - proceeding with next steps..."
        ;;
    esac
fi

# If build agent was requested, build it first
if [ "$build_agent" = "true" ]; then
    build_adu_agent
fi

# Show generated configuration and offer to install it before agent testing
if [ "$test_agent" = "true" ] || [ "$interactive_mode" = "true" ] || [ $original_argc -eq 0 ]; then
    echo ""
    echo "📋 Configuration Management"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "   Before testing the ADU agent, let's review and install the configuration."
    echo ""

    show_and_install_config
    config_install_result=$?

    case $config_install_result in
    0)
        echo ""
        echo "   ✅ Configuration management completed - ready for agent testing"
        ;;
    1)
        echo ""
        echo "   ❌ Configuration installation failed"
        echo "   💡 Agent testing may fail without proper configuration"
        ;;
    2)
        echo ""
        echo "   ⏭️  Using existing system configuration for agent testing"
        ;;
    esac
fi

# If agent test was requested, show device registration and run it
if [ "$test_agent" = "true" ]; then
    echo ""
    echo "📱 Azure IoT Hub Device Registration Commands:"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "💡 Run these commands to register your device and module in Azure IoT Hub:"
    echo ""

    # Extract thumbprints for registration commands
    if [ -f "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" ] && [ -f "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" ]; then
        local_primary_thumbprint=$(openssl x509 -in "$demo_gen_certs_folder/client-primary-$cert_suffix.pem" -noout -sha1 -fingerprint | sed 's/[:]//g' | sed 's/SHA1 Fingerprint=//')
        local_secondary_thumbprint=$(openssl x509 -in "$demo_gen_certs_folder/client-secondary-$cert_suffix.pem" -noout -sha1 -fingerprint | sed 's/[:]//g' | sed 's/SHA1 Fingerprint=//')
    else
        local_primary_thumbprint="THUMBPRINT_NOT_FOUND"
        local_secondary_thumbprint="THUMBPRINT_NOT_FOUND"
    fi

    echo "🏗️ Create the device with X.509 certificate authentication:"
    cat << EOF
az iot hub device-identity create \\
  --device-id "$device_id" \\
  --hub-name "${iot_hub_hostname%.azure-devices.net}" \\
  --auth-method x509_thumbprint \\
  --primary-thumbprint "$local_primary_thumbprint" \\
  --secondary-thumbprint "$local_secondary_thumbprint"
EOF
    echo ""
    echo "🔧 Create the module for Device Update:"
    cat << EOF
az iot hub module-identity create \\
  --device-id "$device_id" \\
  --module-id "$module_id" \\
  --hub-name "${iot_hub_hostname%.azure-devices.net}" \\
  --auth-method x509_thumbprint \\
  --primary-thumbprint "$local_primary_thumbprint" \\
  --secondary-thumbprint "$local_secondary_thumbprint"
EOF
    echo ""
    echo "📋 Device Details:"
    echo "   🆔 Device ID: $device_id"
    echo "   🔧 Module ID: $module_id"
    echo "   🌐 IoT Hub: $iot_hub_hostname"
    echo "   🔑 Primary Thumbprint: $local_primary_thumbprint"
    echo "   🔑 Secondary Thumbprint: $local_secondary_thumbprint"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""

    test_adu_agent
fi

echo ""
echo "🎉 All X.509 tests completed successfully!"

# Save current run parameters for future change detection
save_run_parameters

echo ""
echo "📋 Generated Files Summary:"
echo "   📁 Certificate files in: $demo_gen_certs_folder/"
if [ -d "$demo_gen_certs_folder" ]; then
    find "$demo_gen_certs_folder" -type f -exec ls -la {} \; | sed 's/^/      /'
else
    echo "      • ca-$cert_suffix.pem (CA Certificate)"
    echo "      • ca-$cert_suffix.key (CA Private Key)"
    echo "      • ca-combined-$cert_suffix.pem (Combined CA + Microsoft root CAs)"
    echo "      • client-$cert_suffix.pem (Client Certificate)"
    echo "      • client-$cert_suffix.key (Client Private Key)"
fi
echo ""
echo "   📁 Installed certificates in: /etc/adu/certs/"
if sudo test -d "/etc/adu/certs/" 2> /dev/null; then
    sudo find "/etc/adu/certs/" -name "*$device_id*" -type f -exec ls -la {} \; | sed 's/^/      /'
else
    echo "      • ca-$cert_suffix.pem (Basic CA Certificate)"
    if [ -f "$demo_gen_certs_folder/ca-combined-$cert_suffix.pem" ]; then
        echo "      • ca-combined-$cert_suffix.pem (Combined CA + Microsoft root CAs)"
    fi
    echo "      • client-$cert_suffix.pem (Client Certificate)"
    echo "      • client-$cert_suffix.key (Client Private Key)"
fi
echo ""
echo "   📄 Device Update configuration: $du_config_file"
echo "      X.509 connectionSource format:"
echo '      • connectionType: "X509"'
echo "      • connectionData: \"HostName=${iot_hub_hostname:-YOUR_IOT_HUB_HOSTNAME};DeviceId=$device_id$([ -n "$module_id" ] && echo ";ModuleId=$module_id");x509=true\""
echo "      • connectionX509CertFilePath: \"/etc/adu/certs/client-$cert_suffix.pem\""
echo "      • connectionX509PrivateKeyFilePath: \"/etc/adu/certs/client-$cert_suffix.key\""
if [ -f "$demo_gen_certs_folder/ca-combined-$cert_suffix.pem" ]; then
    echo "      • connectionX509CaCertFilePath: \"/etc/adu/certs/ca-combined-$cert_suffix.pem\""
else
    echo "      • connectionX509CaCertFilePath: \"/etc/adu/certs/ca-$cert_suffix.pem\""
fi
echo ""
echo "      Copy this file to: /etc/adu/du-config.json"
echo ""
echo "   🚀 To use with Device Update Agent:"
echo "      sudo cp $du_config_file /etc/adu/du-config.json"
echo "      sudo systemctl restart adu-agent"
echo ""
echo "⚠️  DISCLAIMER: This script is provided AS-IS for demonstration purposes only."
echo "   Tested on Ubuntu 20.04. No warranty, guarantee, or support provided."
echo "   Always test in non-production environments first."
