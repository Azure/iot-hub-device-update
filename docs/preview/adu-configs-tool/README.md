# Azure Device Update - Certificate Setup

## Quick Start

**Step 1: Add your certificate files**

Drop your certificate files into the `certs/` folder:
- `.pfx` files (recommended - contains certificate + private key)
- `.cer` files (with matching `.key` files)

**Step 2: Generate package**

Run the setup tool:
```bash
python3 setup_device_certs.py
```

This automatically:
- Finds and analyzes certificate files in `certs/` folder
- Extracts device ID from certificate CN
- Reads defaults from `du-config-template.json`
- Generates ready-to-deploy package in `adu-configs-pkg/`

## Files

- `setup_device_certs.py` - Main setup tool
- `du-config-template.json` - Configuration template with defaults
- `requirements.txt` - Python dependencies
- `certs/` - Your certificate files (.pfx, .cer)
- `references/` - Documentation
- `adu-configs-pkg/` - Generated output (created by script)

## Usage

### Basic (no prompts):
```bash
python3 setup_device_certs.py
```

### Without SSH setup:
```bash
python3 setup_device_certs.py --no-ssh
```

### Help:
```bash
python3 setup_device_certs.py --help
```

## Output

The script creates `adu-configs-pkg/` containing:
- `{device-id}-cert.pem` - Device certificate
- `{device-id}-key.pem` - Private key
- `{device-id}-fullchain.pem` - CA certificate chain
- `du-config.json` - ADU agent configuration
- `adu-device-setup.sh` - Automated setup script
- `README.txt` - Detailed instructions

## Deploy to Device

1. Register device in Azure IoT Hub with the thumbprint shown
2. Copy package to device:
   ```bash
   scp -r adu-configs-pkg/ user@device-ip:~/
   ```
3. On device, run:
   ```bash
   sudo ./adu-configs-pkg/adu-device-setup.sh
   ```

## Customize Defaults

Edit `du-config-template.json` to change:
- IoT Hub hostname
- Manufacturer name
- Device model

The script will automatically use these values.

## Requirements

```bash
pip install cryptography
```

## Troubleshooting

**No certificates found:**
- **Place `.pfx` or `.cer` files in the `certs/` folder first**
- Ensure files contain device certificates with private keys
- `.pfx` files should include both certificate and private key
- `.cer` files need matching `.key` file with same base name

**No private key:**
- Use `.pfx` files (contain both certificate and key)
- Or place `.key` file next to `.cer` file with same name

**Wrong IoT Hub:**
- Edit `du-config-template.json` and update the `connectionData` field

## Documentation

See `references/` folder for detailed guides:
- `how-to-x509-authentication.md` - Complete X.509 setup guide
