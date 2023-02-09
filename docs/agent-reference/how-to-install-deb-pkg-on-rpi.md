# Install from Debian packages hosted on packages.microsoft.com

## Raspberry Pi 3

### For ubuntu server 20.04 (64-bit)

1. From RPi Imager, choose `Other general-purpose OS -> Ubuntu Server 20.04.5 LTS (64-bit)`
2. [setup packages.microsoft.com apt.d repository](https://learn.microsoft.com/en-us/windows-server/administration/linux-package-repository-for-microsoft-software#ubuntu).
3. `sudo apt install deviceupdate-agent`

### For Debian 10 armhf on rpi3

1. From RPi Imager, choose `Raspberry Pi OS" - > Raspberry Pi OS Lite (Legacy)`
2. Follow step 1 of [add apt.d for packages.microsoft.com for Debian 10 buster](https://learn.microsoft.com/en-us/windows-server/administration/linux-package-repository-for-microsoft-software#debian).
3. Step 2 fails with "could not find a distribution template for Raspbian/buster". Instead do:
    
    `echo "deb https://packages.microsoft.com/debian/10/prod buster main" > /etc/apt/sources.list.d/microsoft-prod.list`
    
5. `sudo apt update`
6. `sudo apt install deviceupdate-agent`

## Raspberry Pi 4

### Ubuntu 20.04 aarch64

1. From RPi Imager, choose `Other general-purpose OS -> Ubuntu Server 20.04.5 LTS (64-bit)`
2. [setup packages.microsoft.com apt.d repository](https://learn.microsoft.com/en-us/windows-server/administration/linux-package-repository-for-microsoft-software#ubuntu).
3. `sudo apt install deviceupdate-agent`
