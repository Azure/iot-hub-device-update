# Device Update for Azure IoT Hub Tutorials

Tutorials
- [Tutorial 1](#tutorial-1---single-step-update) - Single Step Update
- [Tutorial 2](#tutorial-2---multi-step-update) - Multi Step Update
- [Tutorial 3](#tutorial-3---bundle-update-and-multi-component-update) - Bundle Update and Multi Component Update

---
## Tutorial 1 - Single Inline Step Update

As described in [Multi Step Ordered Execution]() document, a step can be either `inline` or `reference`.

This example update (Contoso.Virtual-Vacuum.1.0) demonstrates how to create an update with a single `inline` step that will install an APT package ('tree 10.7.0-5') onto the target device.

Follow this [instruction](#generate-example-updates) to generate the example update using `CreateSampleMSOEUpdate-1.x.ps1` PowerShell script.

### Example Import Manifest - Contoso.Virtual-Vacuum.1.0

```json
{
    "updateId":  {
                     "provider":  "Contoso",
                     "name":  "Virtual-Vacuum",
                     "version":  "1.0"
                 },
    "isDeployable":  true,
    "compatibility":  [
                          {
                              "model":  "virtual-vacuum-v1",
                              "manufacturer":  "contoso"
                          }
                      ],
    "instructions":  {
                         "steps":  [
                                       {
                                           "type":  "inline",
                                           "description":  "APT update that installs libcurl4-doc package.",
                                           "handler":  "microsoft/apt:1",
                                           "files":  [
                                                         "apt-manifest-1.0.json"
                                                     ],
                                           "handlerProperties":  {
                                                                     "installedCriteria":  "apt-update-test-1.1"
                                                                 }
                                       }
                                   ]
                     },
    "files":  [
                  {
                      "filename":  "apt-manifest-1.0.json",
                      "sizeInBytes":  132,
                      "hashes":  {
                                     "sha256":  "nACI0Obg4Px/f2f85oOko3bHE0HJ8Vlq3CMafx9g2Bo="
                                 }
                  }
              ],
    "createdDateTime":  "2022-05-13T21:57:08.9036596Z",
    "manifestVersion":  "4.0"
}

```

From the example import manifest above, you will notice the following:
- The `instructions.steps` property (array) contains only 1 step.
- The `type` property indicates that this is an inline step.
- A `handler` property informs the Device Update agent which Content Handler to be used to process this step.
- The `files` property contains a single file entity. This file is called an APT manifest. The current implementation of APT Content Handler requires one APT manifest. (See [apt-manifest-1.0.json](./sample-updates/data-files/APT/apt-manifest-1.0.json))
- An `handlerProperties.installedCriteria` is a string used by APT Content Handler to determine whether this 'step' has been installed (or processed) on the target device.

---
## Tutorial 2 - Multi Step Update

A **Multi Step Update** is an update that contains at least two steps. If at least one of those steps is a `reference` step, the update will be called `Bundle Update`.

This example update (Contoso.Virtual-Vacuum.2.2) demonstrates how to create an update with two `inline` steps.
- Step 0 - Install an APT package ('libcurl4-doc') onto the target device.
- Step 1 - Install an APT package ('tree 10.7.0-5') onto the target device.

> NOTE | This demonstrates that you can use two steps to install two APT packages. Alternatively, you can install two packages in a single step by adding both packages' information in a single APT manifest.

Follow this [instruction](#generate-example-updates) to generate the example update using `CreateSampleMSOEUpdate-2.x.ps1` PowerShell script.

### Example Import Manifest - Contoso.Virtual-Vacuum.2.2

```json
{
    "updateId":  {
                     "provider":  "Contoso",
                     "name":  "Virtual-Vacuum",
                     "version":  "2.2"
                 },
    "isDeployable":  true,
    "compatibility":  [
                          {
                              "model":  "virtual-vacuum-v1",
                              "manufacturer":  "contoso"
                          }
                      ],
    "instructions":  {
                         "steps":  [
                                       {
                                           "type":  "inline",
                                           "description":  "Install libcurl4-doc on host device",
                                           "handler":  "microsoft/apt:1",
                                           "files":  [
                                                         "apt-manifest-1.0.json"
                                                     ],
                                           "handlerProperties":  {
                                                                     "installedCriteria":  "apt-update-test-2.2"
                                                                 }
                                       },
                                       {
                                           "type":  "inline",
                                           "description":  "Install tree on host device",
                                           "handler":  "microsoft/apt:1",
                                           "files":  [
                                                         "apt-manifest-tree-1.0.json"
                                                     ],
                                           "handlerProperties":  {
                                                                     "installedCriteria":  "apt-update-test-tree-2.2"
                                                                 }
                                       }
                                   ]
                     },
    "files":  [
                  {
                      "filename":  "apt-manifest-1.0.json",
                      "sizeInBytes":  124,
                      "hashes":  {
                                     "sha256":  "cjJdOa7v4bYyKUrCtF7cSTuPyhfWt8BYkujGtkRDjJ8="
                                 }
                  },
                  {
                      "filename":  "apt-manifest-tree-1.0.json",
                      "sizeInBytes":  138,
                      "hashes":  {
                                     "sha256":  "8vEB0TLIWS6PFkQhJqlcY8ZGiEZGoZALLF9Yx6wP2XA="
                                 }
                  }
              ],
    "createdDateTime":  "2022-05-13T21:47:36.3189977Z",
    "manifestVersion":  "4.0"
}
```

This import manifest contains steps that use 'microsoft/apt:1' handler type. Each step requires a corresponding APT manifest containing information about the package to be installed.

---

## Tutorial 3 - Bundle Update and Multi Component Update

A **Bundle Update** is an update that contains at least one reference step.

A **Multi Component Update** is a Bundle Update intended for a device with a registered `Component Enumerator Extension`.

> If the target device doesn't have a registered component enumerator, the Device Update Agent will assume that every step is intended to be installed onto the host device and will not pass any 'Component Context' to a Content Handler when processing steps.
>
> See [Steps Handler](../../../../../extensions/update_manifest_handlers/steps_handler/) for more information.

Follow this [instruction](#generate-example-updates) to generate the example update using `CreateSampleMSOEUpdate-3.x.ps1` PowerShell script.

**Important** - follow this [instruction](#register-contoso-components-enumerator-extension) to register the example Contoso Component Enumerator and Components Inventory file.

### Example Import Manifest - Contoso.Virtual-Vacuum.3.3

This import manifest contains a single reference step that requires the [contoso Virtual-Vacuum-virtual-camera 1.4](#example-import-manifest-for-reference-update-virtual-vacuum-virtual-camera-14) update to be imported simultaneously.

```json
{
    "updateId":  {
                     "provider":  "Contoso",
                     "name":  "Virtual-Vacuum",
                     "version":  "3.3"
                 },
    "isDeployable":  true,
    "compatibility":  [
                          {
                              "model":  "virtual-vacuum-v1",
                              "manufacturer":  "contoso"
                          }
                      ],
    "instructions":  {
                         "steps":  [
                                       {
                                           "type":  "reference",
                                           "description":  "Cameras Firmware Update",
                                           "updateId":  {
                                                            "provider":  "contoso",
                                                            "name":  "Virtual-Vacuum-virtual-camera",
                                                            "version":  "1.4"
                                                        }
                                       }
                                   ]
                     },
    "createdDateTime":  "2022-09-19T04:02:43.9745720Z",
    "manifestVersion":  "4.0"
}

```

#### Example Import Manifest for the reference update Virtual-vacuum-virtual-camera 1.4

```json
{
    "updateId":  {
                     "provider":  "contoso",
                     "name":  "Virtual-Vacuum-virtual-camera",
                     "version":  "1.4"
                 },
    "isDeployable":  false,
    "compatibility":  [
                          {
                              "group":  "cameras"
                          }
                      ],
    "instructions":  {
                         "steps":  [
                                       {
                                           "type":  "inline",
                                           "description":  "Cameras Update - pre-install step (failure - missing file)",
                                           "handler":  "microsoft/script:1",
                                           "files":  [
                                                         "contoso-camera-installscript.sh"
                                                     ],
                                           "handlerProperties":  {
                                                                     "scriptFileName":  "contoso-camera-installscript.sh",
                                                                     "arguments":  "--pre-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path",
                                                                     "installedCriteria":  "1.1"
                                                                 }
                                       },
                                       {
                                           "type":  "inline",
                                           "description":  "Cameras Update - firmware installation",
                                           "handler":  "microsoft/script:1",
                                           "files":  [
                                                         "contoso-camera-installscript.sh",
                                                         "camera-firmware-1.1.json"
                                                     ],
                                           "handlerProperties":  {
                                                                     "scriptFileName":  "contoso-camera-installscript.sh",
                                                                     "arguments":  "--firmware-file camera-firmware-1.1.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path",
                                                                     "installedCriteria":  "1.1"
                                                                 }
                                       },
                                       {
                                           "type":  "inline",
                                           "description":  "Cameras Update - post-install script",
                                           "handler":  "microsoft/script:1",
                                           "files":  [
                                                         "contoso-camera-installscript.sh"
                                                     ],
                                           "handlerProperties":  {
                                                                     "scriptFileName":  "contoso-camera-installscript.sh",
                                                                     "arguments":  "--post-install-sim-success --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path",
                                                                     "installedCriteria":  "1.1"
                                                                 }
                                       }
                                   ]
                     },
    "files":  [
                  {
                      "filename":  "contoso-camera-installscript.sh",
                      "sizeInBytes":  26884,
                      "hashes":  {
                                     "sha256":  "K6F2TUmY2JPtp6vWI8bNl6sVceacBFTHXIyhOOmS1x8="
                                 }
                  },
                  {
                      "filename":  "camera-firmware-1.1.json",
                      "sizeInBytes":  124,
                      "hashes":  {
                                     "sha256":  "6jst7Roi7dbeb3BNokQo6Gea5qsPPu2KDJ6GotaOQi4="
                                 }
                  }
              ],
    "createdDateTime":  "2022-09-19T04:02:43.9465570Z",
    "manifestVersion":  "4.0"
}

```

### Example Import Manifest - Contoso.Virtual-Vacuum-virtual-camera.1.4

 ```json
{
    "updateId":  {
                     "provider":  "contoso",
                     "name":  "Virtual-Vacuum-virtual-camera",
                     "version":  "1.4"
                 },
    "isDeployable":  false,
    "compatibility":  [
                          {
                              "group":  "cameras"
                          }
                      ],
    "instructions":  {
                         "steps":  [
                                       {
                                           "type":  "inline",
                                           "description":  "Cameras Update - firmware installation",
                                           "handler":  "microsoft/script:1",
                                           "files":  [
                                                         "contoso-camera-installscript.sh",
                                                         "camera-firmware-1.1.json"
                                                     ],
                                           "handlerProperties":  {
                                                                     "scriptFileName":  "contoso-camera-installscript.sh",
                                                                     "arguments":  "--firmware-file camera-firmware-1.1.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path",
                                                                     "installedCriteria":  "1.4"
                                                                 }
                                       }
                                   ]
                     },
    "files":  [
                  {
                      "filename":  "contoso-camera-installscript.sh",
                      "sizeInBytes":  27849,
                      "hashes":  {
                                     "sha256":  "aj2H25P9GnRyAQg4wfiixJToUkDOsch3YQ0fNgHn4as="
                                 }
                  },
                  {
                      "filename":  "camera-firmware-1.1.json",
                      "sizeInBytes":  130,
                      "hashes":  {
                                     "sha256":  "6lYoRUnOVymga/7YH6qSKHv0osG4MFpfSW+naOq7/4M="
                                 }
                  }
              ],
    "createdDateTime":  "2022-05-13T22:49:03.4040790Z",
    "manifestVersion":  "4.0"
}
```

The above update contains a single inline step that installs `camera firmware v1.1` onto every compatible `component` that is currently connected to a host device (as reported by the registered Component Enumerator extension).

When the Component Enumerator extension is registered, the Device Update agent will **select** the compatible `component` by invoking a `SelectComponents` api and passing the referenced-update's compatibility property as a `selector`. For this example, the component enumerator will return every component with a `group` property value that equals `cameras`.

When processing this step, the Agent will iterate through the list of returned compatible components and perform the step's actions (download, install, apply) on each component with each component's context accordingly.
See [Steps Handler](../../../../../extensions/step_handlers/steps_handler/) and [Component Enumerator Extension](../../contoso_component_enumerator/README.md) for more details.

---
## Appendix

### Setup Test Device

Prerequisites

- A test device with Ubuntu 18.04 LTS (or Virtual Machine)
- Connection to the internet
#### Install the Device Update Agent and Dependencies

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

- Restart `deviceupdate-agent` service

```sh
sudo systemctl restart deviceupdate-agent
```

### Optional Steps

The following optional steps are required for Multi Component Update (a.k.a. Proxy Update).

#### Register Contoso Components Enumerator extension

Run following command on your test device

```sh
sudo /usr/bin/AducIotAgent -E /var/lib/adu/extensions/sources/libcontoso_component_enumerator.so
```

#### Setup Mock Components

For testing and demonstration purposes, we'll be creating following mock components on the device:

- 3 motors
- 2 cameras
- hostfs
- rootfs

**IMPORTANT**
This components configuration depends on the implementation of an example Component Enumerator extension called libcontoso_component_enumerator.so, which required a mock component inventory data file `/usr/local/contoso-devices/components-inventory.json`

> Tip: you can copy [demo](../demo) folder to your home directory on the test VM and run `sudo ~/demo/tools/reset-demo-components.sh` to copy required files to the right locations.

#### Add /usr/local/contoso-devices/components-inventory.json

- Copy [components-inventory.json](./demo-devices/contoso-devices/components-inventory.json) to **/usr/local/contoso-devices** folder

### Generate Example Updates

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
```
