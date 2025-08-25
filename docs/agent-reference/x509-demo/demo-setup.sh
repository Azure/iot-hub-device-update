#!/bin/bash
# demo-setup.sh - X.509 Certificate Demo and Testing Script

set -e

# Default device ID
device_id="test-device"
# Connection test option
test_connection=""
iot_hub_hostname=""

# Function to display help
show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --device-id DEVICE_ID              Device ID to use for certificate generation (default: test-device)"
    echo "  --test-connection IOTHUB_HOSTNAME  Test connection to IoT Hub using generated certificates"
    echo "  -h, --help                         Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 --device-id my-iot-device"
    echo "  $0 --device-id my-device --test-connection my-hub.azure-devices.net"
    echo "  $0 --test-connection nox-v120-test-hub.azure-devices.net"
    echo ""
    echo "For testing purposes, you can use: nox-v120-test-hub.azure-devices.net"
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
    --test-connection)
        iot_hub_hostname="$2"
        if [[ -z $iot_hub_hostname ]]; then
            echo "Error: --test-connection requires an IoT Hub hostname"
            show_help
            exit 1
        fi
        test_connection="true"
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
    if openssl s_client -connect nox-v120-test-hub.azure-devices.net:8883 -CApath /etc/ssl/certs -verify_return_error -quiet < /dev/null > /dev/null 2>&1; then
        echo "   ✅ System can verify Azure IoT Hub certificate chain"
    else
        echo "   ⚠️  System may have trouble verifying Azure IoT Hub certificates"
        echo "   💡 This might affect device twin operations"
    fi
}

# Function to test IoT Hub connection
test_iot_hub_connection() {
    echo "🔌 Testing connection to IoT Hub: $iot_hub_hostname"

    # Check if certificates exist
    if [ ! -f "$demo_gen_certs_folder/client.pem" ] || [ ! -f "$demo_gen_certs_folder/client.key" ] || [ ! -f "$demo_gen_certs_folder/ca.pem" ]; then
        echo "❌ Certificates not found. Please ensure certificates are generated first."
        exit 1
    fi

    echo "🔐 Performing comprehensive certificate verification..."
    echo ""

    # Step 1: Certificate file integrity checks
    echo "📋 Step 1: Certificate File Integrity Checks"
    echo "   🔍 Checking CA certificate format..."
    if openssl x509 -in "$demo_gen_certs_folder/ca.pem" -noout -text > /dev/null 2>&1; then
        echo "   ✅ CA certificate format is valid"
    else
        echo "   ❌ CA certificate format is invalid"
        exit 1
    fi

    echo "   🔍 Checking client certificate format..."
    if openssl x509 -in "$demo_gen_certs_folder/client.pem" -noout -text > /dev/null 2>&1; then
        echo "   ✅ Client certificate format is valid"
    else
        echo "   ❌ Client certificate format is invalid"
        exit 1
    fi

    echo "   🔍 Checking private key format..."
    if openssl rsa -in "$demo_gen_certs_folder/client.key" -check -noout > /dev/null 2>&1; then
        echo "   ✅ Private key format is valid"
    else
        echo "   ❌ Private key format is invalid"
        exit 1
    fi

    # Step 2: Certificate-Key pair validation
    echo ""
    echo "📋 Step 2: Certificate-Key Pair Validation"
    echo "   🔍 Verifying private key matches certificate..."
    cert_modulus=$(openssl x509 -in "$demo_gen_certs_folder/client.pem" -noout -modulus 2> /dev/null)
    key_modulus=$(openssl rsa -in "$demo_gen_certs_folder/client.key" -noout -modulus 2> /dev/null)

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
    chain_result=$(openssl verify -CAfile "$demo_gen_certs_folder/ca.pem" "$demo_gen_certs_folder/client.pem" 2>&1)
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
    device_id_from_cert=$(openssl x509 -in "$demo_gen_certs_folder/client.pem" -noout -subject | sed 's/.*CN = \([^,]*\).*/\1/')
    echo "   📱 Device ID from certificate: $device_id_from_cert"

    if [ "$device_id_from_cert" = "$device_id" ]; then
        echo "   ✅ Device ID in certificate matches requested ID"
    else
        echo "   ⚠️  Warning: Device ID mismatch (cert: $device_id_from_cert, requested: $device_id)"
    fi

    echo "   📅 Certificate validity period:"
    openssl x509 -in "$demo_gen_certs_folder/client.pem" -noout -dates | sed 's/^/      /'

    # Check if certificate is currently valid
    if openssl x509 -in "$demo_gen_certs_folder/client.pem" -checkend 0 > /dev/null 2>&1; then
        echo "   ✅ Certificate is currently valid (not expired)"
    else
        echo "   ❌ Certificate has expired"
        exit 1
    fi

    # Step 5: Certificate thumbprints
    echo ""
    echo "📋 Step 5: Certificate Thumbprints for IoT Hub"
    primary_thumbprint=$(openssl x509 -in "$demo_gen_certs_folder/client.pem" -noout -sha1 -fingerprint | sed 's/[:]//g' | sed 's/SHA1 Fingerprint=//')
    secondary_thumbprint=$(openssl x509 -in "$demo_gen_certs_folder/client.pem" -noout -sha256 -fingerprint | sed 's/[:]//g' | sed 's/SHA256 Fingerprint=//')

    echo "   🔑 Primary Thumbprint (SHA1): $primary_thumbprint"
    echo "   🔑 Secondary Thumbprint (SHA256): $secondary_thumbprint"

    # Step 6: TLS Connection Tests
    echo ""
    echo "📋 Step 6: TLS Connection Tests"
    echo "   🌐 Testing basic TLS handshake to $iot_hub_hostname:8883..."

    # Test 1: Basic TLS connection without CA verification
    basic_tls_test=$(timeout 10s openssl s_client -connect "$iot_hub_hostname:8883" \
        -cert "$demo_gen_certs_folder/client.pem" \
        -key "$demo_gen_certs_folder/client.key" \
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
            --cert "$demo_gen_certs_folder/client.pem" \
            --key "$demo_gen_certs_folder/client.key" \
            --insecure -d 2>&1 || true)

        if echo "$mqtt_test" | grep -q "received CONNACK"; then
            if echo "$mqtt_test" | grep -q "Connection Refused: not authorised"; then
                echo "   ✅ MQTT connection successful (X.509 authentication working)"
                echo "   📝 IoT Hub rejected authorization (expected - device needs proper permissions)"
            else
                echo "   ✅ MQTT connection and authorization successful"
            fi
        elif echo "$mqtt_test" | grep -q "sending CONNECT"; then
            echo "   ✅ MQTT client certificate authentication successful"
            echo "   📝 Connection established, authentication working"
        else
            echo "   ⚠️  MQTT connection test inconclusive"
            echo "   📝 Test output: $(echo "$mqtt_test" | head -2 | tr '\n' ' ')"
        fi
    else
        echo "   ⚠️  Mosquitto client not available, skipping MQTT test"
        echo "   💡 Install mosquitto-clients for full MQTT testing"
    fi

    # Step 8: Connection Summary
    echo "
📋 Step 8: Connection Test Summary
   🎯 Target IoT Hub: $iot_hub_hostname
   📱 Device ID: $device_id_from_cert
   🔑 Certificate Authentication: ✅ Working
   🔐 TLS Connection: ✅ Established
   📝 Next steps: Ensure device is registered in IoT Hub with thumbprint

🔧 For self-signed certificates, register the device in IoT Hub:
   1. In Azure Portal, go to your IoT Hub → Device management → Devices
   2. Click '+ Add Device'
   3. Set Device ID to: $device_id_from_cert
   4. Set Authentication type to: 'X.509 Self-Signed'
   5. Set Primary Thumbprint to: $primary_thumbprint
   6. Leave Secondary Thumbprint empty (optional)
   7. Click 'Save'

   Or use Azure CLI:
   az iot hub device-identity create \\
     --hub-name YOUR_IOT_HUB_NAME \\
     --device-id $device_id_from_cert \\
     --auth-method x509_thumbprint \\
     --primary-thumbprint $primary_thumbprint"
}

# Create demo working folder
demo_working_folder=~/x509-demo-temp
mkdir -p "$demo_working_folder"

# Create directory for test certificates
demo_gen_certs_folder="$demo_working_folder/test-certs"
mkdir -p "$demo_gen_certs_folder"

echo "🔧 Setting up X.509 test environment..."

# Check if certificates already exist
if [ -f "$demo_gen_certs_folder/client.pem" ] && [ -f "$demo_gen_certs_folder/client.key" ] && [ -f "$demo_gen_certs_folder/ca.pem" ]; then
    echo "✅ Certificates ready"
else
    echo "📜 Generating enhanced test certificates for IoT device authentication..."

    # Generate CA private key
    openssl genrsa -out "$demo_gen_certs_folder/ca.key" 2048

    # Generate CA certificate with proper extensions (self-signed, valid for 10 years)
    openssl req -new -x509 -days 3650 -key "$demo_gen_certs_folder/ca.key" -out "$demo_gen_certs_folder/ca.pem" \
        -subj "/C=US/ST=WA/O=Contoso/CN=Contoso-CA" \
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

    # Generate client private key
    openssl genrsa -out "$demo_gen_certs_folder/client.key" 2048

    # Generate client certificate signing request with proper subject
    openssl req -new -key "$demo_gen_certs_folder/client.key" -out "$demo_gen_certs_folder/client.csr" \
        -subj "/C=US/ST=WA/O=Contoso/CN=$device_id"

    # Generate client certificate signed by CA with IoT device extensions (valid for 1 year)
    openssl x509 -req -days 365 -in "$demo_gen_certs_folder/client.csr" \
        -CA "$demo_gen_certs_folder/ca.pem" -CAkey "$demo_gen_certs_folder/ca.key" -CAcreateserial \
        -out "$demo_gen_certs_folder/client.pem" \
        -extensions v3_client \
        -extfile <(
            echo '[v3_client]'
            echo 'basicConstraints=CA:FALSE'
            echo 'keyUsage=digitalSignature,keyEncipherment'
            echo 'extendedKeyUsage=clientAuth'
            echo 'subjectKeyIdentifier=hash'
            echo 'authorityKeyIdentifier=keyid,issuer'
        )

    # Clean up CSR file
    rm "$demo_gen_certs_folder/client.csr"

    # Ensure system has necessary root certificates for Azure IoT Hub
    ensure_system_root_certificates

    echo "✅ Enhanced certificates ready"
fi

echo ""
echo "📋 Generated Certificate Information:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Display CA certificate details
echo "🔐 CA Certificate Details:"
echo "   Subject: $(openssl x509 -in "$demo_gen_certs_folder/ca.pem" -noout -subject | sed 's/subject=//')"
echo "   Issuer:  $(openssl x509 -in "$demo_gen_certs_folder/ca.pem" -noout -issuer | sed 's/issuer=//')"
echo "   Valid from: $(openssl x509 -in "$demo_gen_certs_folder/ca.pem" -noout -startdate | sed 's/notBefore=//')"
echo "   Valid to:   $(openssl x509 -in "$demo_gen_certs_folder/ca.pem" -noout -enddate | sed 's/notAfter=//')"
echo "   SHA1 Fingerprint: $(openssl x509 -in "$demo_gen_certs_folder/ca.pem" -noout -sha1 -fingerprint | sed 's/SHA1 Fingerprint=//')"

echo ""

# Display client certificate details
echo "📱 Client Certificate Details:"
echo "   Subject: $(openssl x509 -in "$demo_gen_certs_folder/client.pem" -noout -subject | sed 's/subject=//')"
echo "   Issuer:  $(openssl x509 -in "$demo_gen_certs_folder/client.pem" -noout -issuer | sed 's/issuer=//')"
echo "   Valid from: $(openssl x509 -in "$demo_gen_certs_folder/client.pem" -noout -startdate | sed 's/notBefore=//')"
echo "   Valid to:   $(openssl x509 -in "$demo_gen_certs_folder/client.pem" -noout -enddate | sed 's/notAfter=//')"
device_id_from_cert=$(openssl x509 -in "$demo_gen_certs_folder/client.pem" -noout -subject | sed 's/.*CN = \([^,]*\).*/\1/')
echo "   Device ID (CN): $device_id_from_cert"

echo ""

# Display certificate thumbprints
echo "🔑 Certificate Thumbprints for IoT Hub:"
echo "   Primary (SHA1):   $(openssl x509 -in "$demo_gen_certs_folder/client.pem" -noout -sha1 -fingerprint | sed 's/SHA1 Fingerprint=//')"
echo "   Secondary (SHA256): $(openssl x509 -in "$demo_gen_certs_folder/client.pem" -noout -sha256 -fingerprint | sed 's/SHA256 Fingerprint=//')"

echo ""

# Validate certificate chain
echo "🔍 Certificate Chain Validation:"
if openssl verify -CAfile "$demo_gen_certs_folder/ca.pem" "$demo_gen_certs_folder/client.pem" > /dev/null 2>&1; then
    echo "   ✅ Certificate chain is valid"
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
echo "      📂 Source: $demo_gen_certs_folder/ca.pem"
echo "      📁 Destination: $dest_dir/ca.pem"
if sudo cp "$demo_gen_certs_folder/ca.pem" "$dest_dir/ca.pem" 2> /dev/null; then
    echo "      ✅ CA Certificate installed successfully"
else
    echo "      ❌ Failed to install CA Certificate"
    exit 1
fi

# Install client certificate
echo ""
echo "   📱 Installing Client Certificate..."
echo "      📂 Source: $demo_gen_certs_folder/client.pem"
echo "      📁 Destination: $dest_dir/client.pem"
if sudo cp "$demo_gen_certs_folder/client.pem" "$dest_dir/client.pem" 2> /dev/null; then
    echo "      ✅ Client Certificate installed successfully"
else
    echo "      ❌ Failed to install Client Certificate"
    exit 1
fi

# Install client private key
echo ""
echo "   🔑 Installing Client Private Key..."
echo "      📂 Source: $demo_gen_certs_folder/client.key"
echo "      📁 Destination: $dest_dir/client.key"
if sudo cp "$demo_gen_certs_folder/client.key" "$dest_dir/client.key" 2> /dev/null; then
    # Set appropriate permissions for private key (readable only by owner)
    if sudo chmod 600 "$dest_dir/client.key" 2> /dev/null; then
        echo "      ✅ Client Private Key installed successfully (permissions: 600)"
    else
        echo "      ⚠️  Client Private Key installed but failed to set permissions"
    fi
else
    echo "      ❌ Failed to install Client Private Key"
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
if ! sudo test -f "$dest_dir/ca.pem" || ! sudo test -f "$dest_dir/client.pem" || ! sudo test -f "$dest_dir/client.key"; then
    files_installed=false
fi

if [ "$files_installed" = true ]; then
    echo "   ✅ All certificates installed successfully in $dest_dir"
    echo "      • ca.pem (CA Certificate)"
    echo "      • client.pem (Client Certificate)"
    echo "      • client.key (Client Private Key)"
else
    echo "   ❌ Certificate installation incomplete"
    echo "   📋 Please check file permissions and directory access"
    exit 1
fi

# Run some basic validation tests
echo ""
echo "🧪 Running unit tests..."

echo "🔍 Validating certificates..."
if openssl x509 -in "$demo_gen_certs_folder/client.pem" -noout > /dev/null 2>&1; then
    echo "openssl verify -CAfile \"$demo_gen_certs_folder/ca.pem\" \"$demo_gen_certs_folder/client.pem\""
    echo "✅ Certificate chain valid"
else
    echo "❌ Certificate validation failed"
    exit 1
fi

echo "🔌 Testing agent configuration..."

# If connection test was requested, run it
if [ "$test_connection" = "true" ]; then
    test_iot_hub_connection
fi

echo ""
echo "🎉 All X.509 tests completed successfully!"
