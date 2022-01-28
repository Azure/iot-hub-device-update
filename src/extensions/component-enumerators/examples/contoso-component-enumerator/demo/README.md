# Device Update for Azure IoT Hub tutorial using the Device Update agent to do Proxy Updates

## How To Import Example Updates

> Requirement: perform these steps on machine that supports PowerShell

### Import Example Updates using PowerShell scripts

#### Install and Import ADU PowerShell Modules

- Install PowerShellGet [see details here](https://docs.microsoft.com/en-us/powershell/scripting/gallery/installing-psget?view=powershell-7.1)

```powershell
Install-PackageProvider -Name NuGet -Force

[Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12

Install-Module -Name PowerShellGet -Force -AllowClobber

Update-Module -Name PowerShellGet

```

- Install Azure Az PowerShell Modules

```powershell

Install-Module -Name Az -Scope CurrentUser -Repository PSGallery -Force

```

- Create storage account
- Get container for updates storage

For example:

```powershell

Import-Module .\AduAzStorageBlobHelper.psm1
Import-Module .\AduImportUpdate.psm1

$AzureSubscriptionId = '<YOUR SUBSCRIPTION ID>'
$AzureResourceGroupName = '<YOUR RESOURCE GROUP NAME>'
$AzureStorageAccountName = '<YOUR STORAGE ACCOUNT>'
$AzureBlobContainerName =  '<YOUR BLOB CONTAINER NAME>'

$container = Get-AduAzBlobContainer  -SubscriptionId $AzureSubscriptionId -ResourceGroupName $AzureResourceGroupName -StorageAccountName $AzureStorageAccountName -ContainerName $AzureBlobContainerName

```

- Get REST API token

```powershell
Install-Module MSAL.PS

$AzureAdClientId = '<AZURE AD CLIENT ID>'

$AzureAdTenantId = '<AZURE AD TENANT ID>'

$token = Get-MsalToken -ClientId $AzureAdClientId -TenantId $AzureAdTenantId -Scopes 'https://api.adu.microsoft.com/user_impersonation' -Authority https://login.microsoftonline.com/$AzureAdTenantId/v2.0 
```

#### Generate Example Updates

Go to `sample-updates` directory, then run following commands to generate **all** example updates.  
Note that you can choose to generate only updates you want to try.

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

#### Import Example Updates

Import generated example update using the IoT Hub Device Update portal.

Or, try one of the provided PowerShell scripts below:

```powershell

./ImportSampleMSOEUpdate-1.x.ps1 -AccountEndpoint <your account endpoint url> -InstanceId <your instant id> -BlobContainer $container -AuthorizationToken $token.AccessToken -Verbose

./ImportSampleMSOEUpdate-2.x.ps1 -AccountEndpoint <your account endpoint url> -InstanceId <your instant id> -BlobContainer $container -AuthorizationToken $token.AccessToken -Verbose

./ImportSampleMSOEUpdate-3.x.ps1 -AccountEndpoint <your account endpoint url> -InstanceId <your instant id> -BlobContainer $container -AuthorizationToken $token.AccessToken -Verbose

./ImportSampleMSOEUpdate-4.x.ps1 -AccountEndpoint <your account endpoint url> -InstanceId <your instant id> -BlobContainer $container -AuthorizationToken $token.AccessToken -Verbose

./ImportSampleMSOEUpdate-5.x.ps1 -AccountEndpoint <your account endpoint url> -InstanceId <your instant id> -BlobContainer $container -AuthorizationToken $token.AccessToken -Verbose

./ImportSampleMSOEUpdate-10.x.ps1 -AccountEndpoint <your account endpoint url> -InstanceId <your instant id> -BlobContainer $container -AuthorizationToken $token.AccessToken -Verbose

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

> Tip: you can copy [`demo`](../demo) folder to your home directory on the test VM and run `~/demo/tools/reset-demo-components.sh` to copy required files to the right locations.

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
