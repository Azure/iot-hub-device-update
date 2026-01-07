#!/usr/bin/env python3
"""
Azure Device Update - Certificate Setup Tool
Single command to analyze certificates and generate device setup package
"""

import os
import sys
import json
import hashlib
import base64
from pathlib import Path
from datetime import datetime
from cryptography import x509
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.serialization import pkcs12


def load_certificates_from_pfx(file_path, password=None):
    """Load certificates and private key from .pfx file"""
    with open(file_path, 'rb') as f:
        pfx_data = f.read()
        
    if password is None:
        password = b''
    elif isinstance(password, str):
        password = password.encode()
    
    try:
        private_key, certificate, additional_certs = pkcs12.load_key_and_certificates(
            pfx_data, password, default_backend()
        )
        certs = [certificate] if certificate else []
        if additional_certs:
            certs.extend(additional_certs)
        return certs, private_key
    except Exception as e:
        return [], None


def load_certificate_from_cer(file_path):
    """Load certificate from .cer/.pem file"""
    with open(file_path, 'rb') as f:
        cert_data = f.read()
        
        if b'-----BEGIN CERTIFICATE-----' in cert_data:
            try:
                return x509.load_pem_x509_certificate(cert_data, default_backend())
            except:
                pass
        
        try:
            return x509.load_der_x509_certificate(cert_data, default_backend())
        except:
            pass
        
        try:
            decoded = base64.b64decode(cert_data)
            return x509.load_der_x509_certificate(decoded, default_backend())
        except:
            pass
        
        raise ValueError("Unable to parse certificate file")


def get_certificate_cn(cert):
    """Get Common Name from certificate"""
    try:
        return cert.subject.get_attributes_for_oid(x509.NameOID.COMMON_NAME)[0].value
    except:
        return None


def is_ca_certificate(cert):
    """Check if certificate is a CA certificate"""
    try:
        basic_constraints = cert.extensions.get_extension_for_oid(
            x509.ExtensionOID.BASIC_CONSTRAINTS
        )
        return basic_constraints.value.ca
    except:
        return False


def export_certificate_to_pem(cert, output_path):
    """Export certificate to PEM format"""
    pem = cert.public_bytes(serialization.Encoding.PEM)
    with open(output_path, 'wb') as f:
        f.write(pem)


def export_private_key_to_pem(private_key, output_path):
    """Export private key to PEM format (unencrypted)"""
    pem = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption()
    )
    with open(output_path, 'wb') as f:
        f.write(pem)


def create_full_chain(certs, output_path):
    """Create full certificate chain PEM file"""
    with open(output_path, 'wb') as f:
        for cert in certs:
            pem = cert.public_bytes(serialization.Encoding.PEM)
            f.write(pem)


def generate_du_config(device_id, iothub_hostname, cert_basename, manufacturer, model):
    """Generate du-config.json content"""
    config = {
        "schemaVersion": "1.2",
        "aduShellTrustedUsers": ["adu", "do"],
        "manufacturer": "device_info_manufacturer",
        "model": "device_info_model",
        "agents": [
            {
                "name": "host-update",
                "runas": "adu",
                "connectionSource": {
                    "connectionType": "X509",
                    "connectionData": f"HostName={iothub_hostname};DeviceId={device_id};x509=true",
                    "connectionX509CertFilePath": f"/etc/adu/certs/{cert_basename}-cert.pem",
                    "connectionX509PrivateKeyFilePath": f"/etc/adu/certs/{cert_basename}-key.pem",
                    "connectionX509CaCertFilePath": f"/etc/adu/certs/{cert_basename}-fullchain.pem"
                },
                "manufacturer": manufacturer,
                "model": model,
                "IdlePauseMilliseconds": "300000"
            }
        ]
    }
    return config


def generate_setup_script(cert_basename, enable_ssh=True):
    """Generate adu-device-setup.sh script"""
    ssh_setup = """# Enable SSH on boot
if [ -d "/boot" ]; then
    touch /boot/ssh
    echo -e "${GREEN}[OK] Created /boot/ssh file${NC}"
elif [ -d "/boot/firmware" ]; then
    touch /boot/firmware/ssh
    echo -e "${GREEN}[OK] Created /boot/firmware/ssh file${NC}"
fi

# Ensure SSH service is enabled
if command -v systemctl &> /dev/null; then
    systemctl enable ssh 2>/dev/null || true
    echo -e "${GREEN}[OK] SSH service enabled${NC}"
fi
echo ""
""" if enable_ssh else ""

    script = f"""#!/bin/bash
#
# Azure Device Update - Device Setup Script
# Generated on: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
#

set -e

RED='\\033[0;31m'
GREEN='\\033[0;32m'
YELLOW='\\033[1;33m'
BLUE='\\033[0;34m'
NC='\\033[0m'

echo "=================================================="
echo "Azure Device Update - Device Setup"
echo "=================================================="
echo ""

if [ "$EUID" -ne 0 ]; then 
    echo -e "${{RED}}[ERROR] This script must be run as root (use sudo)${{NC}}"
    exit 1
fi

SCRIPT_DIR="$( cd "$( dirname "${{BASH_SOURCE[0]}}" )" && pwd )"
echo -e "${{BLUE}}[INFO] Script directory: $SCRIPT_DIR${{NC}}"
echo ""

{ssh_setup}
# Create ADU user if it doesn't exist
if ! id "adu" &>/dev/null; then
    echo -e "${{YELLOW}}[CONFIG] Creating 'adu' user...${{NC}}"
    useradd --system --user-group --shell /bin/false adu
    echo -e "${{GREEN}}[OK] User 'adu' created${{NC}}"
else
    echo -e "${{GREEN}}[OK] User 'adu' already exists${{NC}}"
fi
echo ""

# Setup certificates
echo -e "${{YELLOW}}[CONFIG] Installing certificates...${{NC}}"
mkdir -p /etc/adu/certs

if [ ! -f "$SCRIPT_DIR/{cert_basename}-cert.pem" ]; then
    echo -e "${{RED}}[ERROR] Certificate files not found in $SCRIPT_DIR${{NC}}"
    exit 1
fi

cp "$SCRIPT_DIR/{cert_basename}-cert.pem" /etc/adu/certs/
cp "$SCRIPT_DIR/{cert_basename}-key.pem" /etc/adu/certs/
cp "$SCRIPT_DIR/{cert_basename}-fullchain.pem" /etc/adu/certs/

chmod 644 /etc/adu/certs/{cert_basename}-cert.pem
chmod 644 /etc/adu/certs/{cert_basename}-fullchain.pem
chmod 600 /etc/adu/certs/{cert_basename}-key.pem
chown -R adu:adu /etc/adu/certs
echo -e "${{GREEN}}[OK] Certificates installed${{NC}}"
echo ""

# Install configuration
echo -e "${{YELLOW}}[CONFIG] Installing device configuration...${{NC}}"
mkdir -p /etc/adu
cp "$SCRIPT_DIR/du-config.json" /etc/adu/
chown adu:adu /etc/adu/du-config.json
chmod 644 /etc/adu/du-config.json
echo -e "${{GREEN}}[OK] Configuration installed${{NC}}"
echo ""

# Display thumbprint
echo -e "${{BLUE}}[INFO] Certificate Thumbprint for IoT Hub registration:${{NC}}"
THUMBPRINT=$(openssl x509 -in /etc/adu/certs/{cert_basename}-cert.pem -noout -sha1 -fingerprint | sed 's/[:]//g' | sed 's/SHA1 Fingerprint=//')
echo -e "${{GREEN}}SHA1: $THUMBPRINT${{NC}}"
echo ""

echo "=================================================="
echo -e "${{GREEN}}[SUCCESS] Device setup completed!${{NC}}"
echo "=================================================="
echo ""
echo -e "${{YELLOW}}[NEXT STEPS]${{NC}}"
echo "1. Register device in Azure IoT Hub with thumbprint: $THUMBPRINT"
echo "2. Install ADU agent: sudo apt install deviceupdate-agent"
echo "3. Start agent: sudo systemctl start deviceupdate-agent.service"
echo ""
"""
    return script


def main():
    # Check for --help or -h
    if '--help' in sys.argv or '-h' in sys.argv:
        print("""
Azure Device Update - Certificate Setup Tool

Usage: python3 setup_device_certs.py [OPTIONS]

Options:
  --no-ssh        Don't enable SSH in setup script
  -h, --help      Show this help message

This tool automatically:
1. Analyzes certificate files in the certs/ folder
2. Selects the appropriate device certificate
3. Reads defaults from du-config-template.json
4. Generates a complete device setup package

Output: adu-configs-pkg/ folder ready to copy to your device
""")
        sys.exit(0)
    
    enable_ssh = '--no-ssh' not in sys.argv
    
    print("=" * 80)
    print("Azure Device Update - Certificate Setup")
    print("=" * 80)
    print()
    
    script_dir = Path(__file__).parent
    certs_dir = script_dir / "certs"
    
    if not certs_dir.exists():
        certs_dir = script_dir
    
    print(f"[1/5] Scanning for certificates in: {certs_dir}")
    
    # Find certificate files
    device_certs = []
    ca_certs = []
    
    for file_path in certs_dir.rglob('*'):
        if not file_path.is_file():
            continue
        
        ext = file_path.suffix.lower()
        
        if ext == '.pfx':
            certs, private_key = load_certificates_from_pfx(file_path)
            if certs:
                for cert in certs:
                    cn = get_certificate_cn(cert)
                    is_ca = is_ca_certificate(cert)
                    
                    if is_ca:
                        ca_certs.append({'cert': cert, 'cn': cn, 'file': file_path, 'key': None})
                    else:
                        device_certs.append({'cert': cert, 'cn': cn, 'file': file_path, 'key': private_key})
        
        elif ext in ['.cer', '.crt', '.pem']:
            try:
                cert = load_certificate_from_cer(file_path)
                cn = get_certificate_cn(cert)
                is_ca = is_ca_certificate(cert)
                
                if is_ca:
                    ca_certs.append({'cert': cert, 'cn': cn, 'file': file_path, 'key': None})
                else:
                    key_file = file_path.with_suffix('.key')
                    private_key = None
                    if key_file.exists():
                        try:
                            with open(key_file, 'rb') as f:
                                private_key = serialization.load_pem_private_key(
                                    f.read(), password=None, backend=default_backend()
                                )
                        except:
                            pass
                    device_certs.append({'cert': cert, 'cn': cn, 'file': file_path, 'key': private_key})
            except:
                pass
    
    print(f"      Found: {len(device_certs)} device cert(s), {len(ca_certs)} CA cert(s)")
    
    if not device_certs:
        print("\n[ERROR] No device certificates found!")
        sys.exit(1)
    
    # Auto-select device certificate with private key
    selected_cert = None
    for cert_info in device_certs:
        if cert_info['key']:
            selected_cert = cert_info
            break
    
    if not selected_cert:
        selected_cert = device_certs[0]
    
    if not selected_cert['key']:
        print(f"\n[ERROR] No private key found for certificate: {selected_cert['cn']}")
        sys.exit(1)
    
    device_id = selected_cert['cn']
    print(f"      Selected: {device_id} (from {selected_cert['file'].name})")
    print()
    
    # Load defaults from template
    print(f"[2/5] Loading configuration defaults...")
    template_path = script_dir / "du-config-template.json"
    
    default_hostname = "adu-bugbash-01062026-iothub.azure-devices.net"
    default_manufacturer = "Contoso"
    default_model = "Smart-Box"
    
    if template_path.exists():
        try:
            with open(template_path, 'r') as f:
                template_config = json.load(f)
                agent_config = template_config.get('agents', [{}])[0]
                
                connection_data = agent_config.get('connectionSource', {}).get('connectionData', '')
                if 'HostName=' in connection_data:
                    hostname_part = connection_data.split('HostName=')[1].split(';')[0].split('DeviceId=')[0]
                    if hostname_part:
                        default_hostname = hostname_part
                
                if 'manufacturer' in agent_config:
                    default_manufacturer = agent_config['manufacturer']
                if 'model' in agent_config:
                    default_model = agent_config['model']
            
            print(f"      IoT Hub: {default_hostname}")
            print(f"      Manufacturer: {default_manufacturer}")
            print(f"      Model: {default_model}")
        except Exception as e:
            print(f"      Using default values")
    else:
        print(f"      Template not found, using defaults")
    print()
    
    # Build certificate chain
    print(f"[3/5] Building certificate chain...")
    ca_chain = []
    current_issuer = selected_cert['cert'].issuer.rfc4514_string()
    depth = 0
    max_depth = 10
    
    print(f"      Device: {device_id}")
    
    while depth < max_depth:
        found_issuer = False
        for ca_info in ca_certs:
            ca_subject = ca_info['cert'].subject.rfc4514_string()
            if ca_subject == current_issuer:
                ca_cn = get_certificate_cn(ca_info['cert'])
                is_self_signed = ca_info['cert'].subject.rfc4514_string() == ca_info['cert'].issuer.rfc4514_string()
                
                ca_chain.append(ca_info['cert'])
                
                if is_self_signed:
                    print(f"      Root CA: {ca_cn}")
                    found_issuer = False
                    break
                else:
                    print(f"      Intermediate CA: {ca_cn}")
                    current_issuer = ca_info['cert'].issuer.rfc4514_string()
                    found_issuer = True
                    break
        
        if not found_issuer:
            break
        
        depth += 1
    print()
    
    # Generate package
    print(f"[4/5] Generating device package...")
    output_dir = script_dir / "adu-configs-pkg"
    output_dir.mkdir(exist_ok=True)
    
    cert_basename = device_id.replace(' ', '-').replace('/', '-')
    
    # Export files
    cert_path = output_dir / f"{cert_basename}-cert.pem"
    key_path = output_dir / f"{cert_basename}-key.pem"
    fullchain_path = output_dir / f"{cert_basename}-fullchain.pem"
    
    export_certificate_to_pem(selected_cert['cert'], cert_path)
    export_private_key_to_pem(selected_cert['key'], key_path)
    create_full_chain(ca_chain if ca_chain else [selected_cert['cert']], fullchain_path)
    
    # Generate config
    config = generate_du_config(device_id, default_hostname, cert_basename, default_manufacturer, default_model)
    config_path = output_dir / "du-config.json"
    with open(config_path, 'w') as f:
        json.dump(config, f, indent=4)
    
    # Generate setup script
    setup_script = generate_setup_script(cert_basename, enable_ssh)
    setup_script_path = output_dir / "adu-device-setup.sh"
    with open(setup_script_path, 'w') as f:
        f.write(setup_script)
    os.chmod(setup_script_path, 0o755)
    
    print(f"      Created {len(list(output_dir.iterdir()))} files")
    print()
    
    # Calculate thumbprint
    print(f"[5/5] Certificate thumbprint for IoT Hub registration:")
    with open(cert_path, 'rb') as f:
        cert_data = f.read()
        cert_obj = x509.load_pem_x509_certificate(cert_data, default_backend())
        thumbprint = hashlib.sha1(cert_obj.public_bytes(serialization.Encoding.DER)).hexdigest().upper()
        print(f"      SHA1: {thumbprint}")
    print()
    
    print("=" * 80)
    print("[SUCCESS] Package ready!")
    print("=" * 80)
    print()
    print(f"Package location: {output_dir}/")
    print()
    print("Quick Start:")
    print(f"  1. Register device '{device_id}' in IoT Hub with thumbprint above")
    print(f"  2. Copy package to device: scp -r {output_dir.name}/ user@device:~/")
    print(f"  3. On device run: sudo ./{output_dir.name}/adu-device-setup.sh")
    print()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n[CANCELLED]")
        sys.exit(1)
    except Exception as e:
        print(f"\n[ERROR] {e}")
        sys.exit(1)
