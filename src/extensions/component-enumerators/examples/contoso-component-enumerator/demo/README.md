# Device Update for Azure IoT Hub tutorial using the Device Update agent to do Proxy Updates

## Generate Example Updates

Open PowerShell terminal, go [[project-root]/tools/AduCmdlets](../../../../../../tools/AduCmdlets/) directory, then import AduUpdate.psm1 module:

```powershell
Import-Module ./AduUpdate.psm1
```

Next, go to [sample-updates](./sample-updates/) directory, then run following commands to generate **all** example updates:

> NOTE | You can choose to generate only updates you want to try.

```powershell
./CreateSampleMSOEUpdate-1.x.ps1
./CreateSampleMSOEUpdate-2.x.ps1
./CreateSampleMSOEUpdate-3.x.ps1
./CreateSampleMSOEUpdate-4.x.ps1
./CreateSampleMSOEUpdate-5.x.ps1
./CreateSampleMSOEUpdate-6.x.ps1
./CreateSampleMSOEUpdate-7.x.ps1
./CreateSampleMSOEUpdate-8.x.ps1
./CreateSampleMSOEUpdate-10.x.ps1
```

## Setup Test Device

### Prerequisites

- Ubuntu 18.04 LTS Server

### Install the Device Update Agent and Dependencies

- Register packages.microsoft.com in APT package repository

```sh
curl https://packages.microsoft.com/config/ubuntu/18.04/multiarch/prod.list > ~/microsoft-prod.list

sudo cp ~/microsoft-prod.list /etc/apt/sources.list.d/

curl https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > ~/microsoft.gpg

sudo cp ~/microsoft.gpg /etc/apt/trusted.gpg.d/

sudo apt-get update
```

- Install test **deviceupdate-agent-[version].deb** package on the test device.  
e.g.

  ```sh
  sudo apt-get install ./deviceupdate-agent-0.8.0-public-preview-refresh.deb
  ```

Note: this will automatically install the delivery-optimization-agent package from packages.microsoft.com

- Specify a connection string in /etc/adu/du-config.json

>NOTE | The current example only compatible with device with "contoso" manufacturer, and "virtual-vacuum-v1" model. These values must be correctly set inside `agents` entry in du-config.json

- Ensure that /etc/adu/du-diagnostics-config.json contain correct settings.  
  e.g.  

```sh
{
  "logComponents":[
    {
      "componentName":"adu",
      "logPath":"/var/log/adu/"
    },
    {
      "componentName":"do",
      "logPath":"/var/log/deliveryoptimization-agent/"
    }
  ],
  "maxKilobytesToUploadPerLogPath":50
}
```

- Restart `adu-agent` service

```sh
sudo systemctl restart adu-agent
```

### Setup Mock Components

For testing and demonstration purposes, we'll be creating following mock components on the device:

- 3 motors
- 2 cameras
- hostfs
- rootfs

**IMPORTANT**  
This components configuration depends on the implementation of an example Component Enumerator extension called libcontoso-component-enumerator.so, which required a mock component inventory data file `/usr/local/contoso-devices/components-inventory.json`

> Tip: you can copy [demo](../demo) folder to your home directory on the test VM and run `~/demo/tools/reset-demo-components.sh` to copy required files to the right locations.

#### Add /usr/local/contoso-devices/components-inventory.json

- Copy [components-inventory.json](./demo-devices/contoso-devices/components-inventory.json) to **/usr/local/contoso-devices** folder
  
#### Register Contoso Components Enumerator extension

- Copy [libcontoso-component-enumerator.so](./sample-components-enumerator/libcontoso-component-enumerator.so) to /var/lib/adu/extensions/sources folder
- Register the extension

```sh
sudo /usr/bin/AducIotAgent -E /var/lib/adu/extensions/sources/libcontoso-component-enumerator.so
```

**Congratulations!** Your VM should now support Proxy Updates!

## Next Step

Try [Tutorial #1 - Install 'tree' Debian package on the Contoso Virtual Vacuum devices, and update their motors' firmware to version 1.1](./tutorial1.md)
