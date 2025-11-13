# ADU Support Tools

A collection of support and diagnostic tools for Azure Device Update (ADU) for IoT Hub.

## 📦 Installation

### Quick Install

Run the installer script from any directory where you want to install the tools:

```bash
curl -sSL https://raw.githubusercontent.com/azure/iot-hub-device-update/adu-support-tools/adu-support-tools/install.sh | bash
```

Or download and run manually:

```bash
wget https://raw.githubusercontent.com/azure/iot-hub-device-update/adu-support-tools/adu-support-tools/install.sh
chmod +x install.sh
./install.sh
```

The installer will:
- Perform a sparse checkout of only the `adu-support-tools` folder
- Let you choose installation location (current directory or custom path)
- Make all scripts executable
- Create a marker file to prevent duplicate installations

---

## 🛠️ Available Tools

### 1. `install.sh`

**Purpose:** Installs ADU Support Tools via sparse Git checkout.

**Features:**
- Clones only the necessary folder from the repository
- Interactive installation path selection
- Prevents installation within existing ADU Support Tools directory
- Automatically makes scripts executable

**Usage:**
```bash
./install.sh
# Follow the interactive prompts to choose installation location
```

**Options:**
- Option 1: Install to current directory
- Option 2: Specify custom installation path

**Example:**
```bash
$ ./install.sh
========================================
ADU Support Tools Installer
========================================

Source: https://github.com/azure/iot-hub-device-update.git
Branch: adu-support-tools
Folder: adu-support-tools

Installation Options:
  1. Current directory: /home/user/projects
  2. Specify custom path

Choose option (1/2): 1
```

---

### 2. `add-packages-microsoft-com-apt-repo.sh`

**Purpose:** Interactively adds the Microsoft APT package repository to Ubuntu/Debian systems.

**Features:**
- Auto-detects OS and version (Ubuntu/Debian)
- Downloads and installs official Microsoft repository package
- Installs prerequisites (wget, apt-transport-https)
- Updates package lists after installation
- Displays available Microsoft packages including ADU components

**Usage:**
```bash
./add-packages-microsoft-com-apt-repo.sh
```

**What it installs:**
- Microsoft GPG signing key
- Microsoft APT repository configuration
- Enables installation of packages like:
  - `dotnet-sdk-8.0`
  - `powershell`
  - `azure-cli`
  - `deviceupdate-agent` (latest: 1.2.0)
  - `deliveryoptimization-agent` (latest: 1.1.0)
  - `aziot-identity-service` (latest: 1.5.6-1)

**Example:**
```bash
$ ./add-packages-microsoft-com-apt-repo.sh
========================================
Microsoft APT Repository Setup
========================================

Detected OS: ubuntu
Version: 22.04 (jammy)

Configuration:
  Repository URL: https://packages.microsoft.com/config/ubuntu/22.04/packages-microsoft-prod.deb

Do you want to proceed with the installation? (y/n): y

# After successful installation, you can install ADU components:
$ sudo apt-get update
$ sudo apt-get install deviceupdate-agent
```

---

### 3. `create-adu-support-bundle.sh`

**Purpose:** Collects ADU logs, configuration, and system information into a compressed support bundle for troubleshooting.

**Features:**
- Collects logs from multiple ADU client configurations:
  - Raspberry Pi Device: `/adu/logs`
  - Packaged client: `/var/log/adu`
  - Delivery Optimization: `/var/cache/do-client-lite/log`
  - Simulator: `/tmp/aduc-logs`
- Gathers system information (OS, kernel, disk, memory)
- Captures service status for ADU and DO agents
- Includes configuration files from `/etc/adu` and `/etc/deliveryoptimization-agent`
- Creates timestamped archive with detailed inventory
- **Privacy-aware**: Displays warning and requires explicit consent
- Generates README.txt with complete file inventory

**Usage:**
```bash
./create-adu-support-bundle.sh
```

**Output:**
- Creates bundle in `./adu-support-bundles/` directory relative to script location
- Archive format: `adu-support-bundle-{hostname}-{timestamp}.tar.gz`
- External README: `adu-support-bundle-{hostname}-{timestamp}-README.txt`

**Example:**
```bash
$ ./create-adu-support-bundle.sh
========================================
ADU Support Bundle Creator
========================================

⚠ PRIVACY AND SECURITY NOTICE
========================================

This script collects system information and logs that may contain:
  • System configuration details
  • Device identifiers and hostnames
  • Network information
  • Service credentials or tokens
  • File paths and directory structures
  • User-specific data

Before sharing this bundle with Microsoft or others:
  1. Review the contents of the generated bundle
  2. Remove any sensitive or confidential information
  3. Verify no credentials or secrets are included

A README.txt will be generated listing all collected files.

Do you understand and agree to proceed? (y/n): y

# ... collection process ...

========================================
✓ Support Bundle Created Successfully!
========================================

Bundle location: ./adu-support-bundles/adu-support-bundle-mydevice-20251113-143022.tar.gz
Bundle size: 2.5M
Log locations found: 3

⚠ IMPORTANT - REVIEW BEFORE SHARING:
  1. Extract and review: tar -xzf ./adu-support-bundles/adu-support-bundle-mydevice-20251113-143022.tar.gz
  2. Check README.txt for list of collected files
  3. Remove any sensitive information from the bundle
  4. Re-package if needed: cd ./adu-support-bundles && tar -czf adu-support-bundle-mydevice-20251113-143022.tar.gz adu-support-bundle-mydevice-20251113-143022/
```

**Bundle Contents:**
- `adu-logs/` - Logs from `/adu/logs` (Raspberry Pi)
- `var-log-adu/` - Logs from `/var/log/adu` (packaged client)
- `do-client-lite-cache/` - Delivery Optimization logs
- `simulator-logs/` - Simulator logs from `/tmp/aduc-logs`
- `system-info/` - OS, kernel, disk, memory, service status
- `configuration/` - ADU and DO configuration files
- `README.txt` - Complete inventory with file paths and sizes

**Privacy Notice:**
This tool collects diagnostic information that may include sensitive data. Always review the bundle contents before sharing with support or third parties. The script requires explicit user consent before proceeding.

---

## 📋 Requirements

- **OS:** Ubuntu 18.04+, Debian 10+, or compatible Linux distributions
- **Tools:** bash, git (for installer), wget, tar
- **Permissions:** sudo access may be required for some operations

---

## 🔧 Troubleshooting

### Git Sparse Checkout Issues

If the installer fails with sparse checkout errors, ensure you have Git 2.25+ installed:

```bash
git --version
```

Upgrade if needed:
```bash
sudo add-apt-repository ppa:git-core/ppa
sudo apt-get update
sudo apt-get install git
```

### Permission Denied Errors

Some log collection operations require sudo access. The scripts will automatically attempt to use sudo when needed.

### No Logs Found

If the support bundle script reports no logs found, verify:
- ADU agent is installed: `dpkg -l | grep deviceupdate`
- ADU service is running: `systemctl status deviceupdate-agent`
- Logs exist in expected locations

---

## 🤝 Contributing

This toolset is part of the [Azure IoT Hub Device Update](https://github.com/azure/iot-hub-device-update) project.

To contribute:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request to the `adu-support-tools` branch

---

## 📄 License

These tools are provided as part of the Azure IoT Hub Device Update project and follow the same license terms.

---

## 📞 Support

For issues or questions:
- Open an issue in the [iot-hub-device-update repository](https://github.com/azure/iot-hub-device-update/issues)
- For Microsoft support customers, attach support bundles to your support case

---

## 📚 Additional Resources

- [Azure Device Update Documentation](https://docs.microsoft.com/azure/iot-hub-device-update/)
- [Device Update Agent GitHub](https://github.com/azure/iot-hub-device-update)
- [Microsoft Package Repository](https://packages.microsoft.com/)
