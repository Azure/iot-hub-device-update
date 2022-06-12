# Running the Device Update for IoT Hub Reference Agent

## Required Dependencies

1. Build the Device Update agent and its dependencies using these [instructions](./how-to-build-agent-code.md)
2. Follow the instructions from [this](https://github.com/microsoft/do-client#delivery-optimization-client) page to build and install the Delivery Optimization agent and plug-in. Note: Delivery Optimization SDK is already installed as part of step 1 above.

## Command Line

```bash
AducIotAgent [options...] '<IoT device connection string>'
AducIotAgent [options...] -- [one or more of connection string and simulator or other additional args]
```

e.g.

```bash
AducIotAgent --enable-iothub-tracing --log-level 1 '<IoT device connection string>'
AducIotAgent --enable-iothub-tracing --log-level 1 -- '<IoT device connection string>'
```

#### Simulator Update Handler

The Simulator Update Handler can be used for demonstration and testing purposes.

See [how to simulate update result](./how-to-simulate-update-result.md) for more details.

### Options Details

`--version` tells the reference agent to output the version.

`--enable-iothub-tracing` tells the reference agent to output verbose tracing from
the Azure IoT C SDK. This option is useful for troubleshooting connection
issues.

`--health-check` tells the reference agent to turn on Health Check, which performs
necessary checks to determine whether ADU Agent can function properly.
Currently, the script is performing the following:
    - Implicitly check that agent process launched successfully.
    - Check that the agent can obtain the connection info.

`--log-level` (argument required) sets the log level of the reference agent's output.
Expected value:
    - 0: Debug
    - 1: Info
    - 2: Warning
    - 3: Error

#### --deviceinfo_manufacturer=\<manufacturer>

This option changes the value of 'manufacturer' that is reported through the
DeviceInformation Digital Twin interface. This value can be used to override the default value of "Contoso".

#### --deviceinfo_model=\<model>

This option changes the value of 'model' that is reported through the
DeviceInformation Digital Twin interface. This value can be used to override the default value of "Virtual Machine".

#### --deviceinfo_swversion=\<version>

This option changes the value of 'swVersion' that is reported through the
DeviceInformation Digital Twin interface.

## Daemon

When the Device Update Agent has been installed as a daemon, it will automatically start
on boot. The Device Update Agent daemon reads the IoT Hub device connection string from a
configuration file. Place your device connection string in /adu/adu-conf.txt.

To manually start the daemon:

```shell
sudo systemctl start deviceupdate-agent
```

To manually stop the daemon:

```shell
sudo systemctl stop deviceupdate-agent
```

## How To Create 'adu' Group and User
IMPORTANT: The Device Update agent must be run as 'adu' user.

Create 'adu' group and 'adu' user by follow these steps:

1. Add 'adu' group
	```shell
    sudo addgroup --system "adu"
    ```

2. Add 'adu' user (with no shell, and no login)
	```shell
    sudo adduser --system "adu" --ingroup "adu" --no-create-home --shell /bin/false
    ```

3. Add 'adu' user to the 'syslog' group to allow the Agent to write to /var/log folder
	```shell
    sudo usermod -aG "syslog" "adu"
    ```

4. Add 'adu' user to the 'do' group to allow access to Delivery Optimization resources
   ```shell
   sudo usermod -aG "do" "adu"
   ```

5. Add 'do' user to the 'adu' group to allow Delivery Optimization agent to download files to the Device Update sandbox folder
   ```shell
   sudo usermod -aG "adu" "do"
   sudo systemctl restart deliveryoptimization-agent
   ```

6. If using IoT Identity Service, add 'adu' user to the following groups.
 Learn more about [how to provision Device Update Agent with IoT Identity Service](https://docs.microsoft.com/azure/iot-hub-device-update/device-update-agent-provisioning#how-to-provision-the-device-update-agent-as-a-module-identity)

   ```shell
    sudo usermod -aG "aziotid" "adu"
    sudo usermod -aG "aziotcs" "adu"
    sudo usermod -aG "aziotks" "adu"
   ```

## Set adu-shell file permissions to perform download and install tasks with "root" privilege

1. Install adu-shell to /usr/lib/adu/.
2. Run following commands
   ```shell
	sudo chown "root:adu" "/usr/lib/adu/adu-shell"
	sudo chmod u=rxs "/usr/lib/adu/adu-shell"
    ```
