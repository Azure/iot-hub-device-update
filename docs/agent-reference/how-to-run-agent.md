# Running the Device Update for IoT Hub Reference Agent

## Command Line

```bash
AducIotAgent '<IoT device connection string>' [options...]
```

e.g.

```bash
AducIotAgent '<IoT device connection string>' --enable-iothub-tracing --log-level 1
```

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

### Simulator Specific Arguments

These arguments are specific to the agent simulator.

#### --simulation_mode=\<mode>

Tells the agent where and if to force a failure. This is useful for evaluating
and testing how Device Update for IoT Hub reports errors.

`mode` can be one of the following options:

* `downloadfailed`
* `installationfailed`
* `applyfailed`
* `allsuccessful`

If no mode is specified, `allsuccessful` is the default.

#### --perform_download

This option will cause the simulator agent to download and verify the hash of
the update.

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
sudo systemctl start adu-agent
```

To manually stop the daemon:

```shell
sudo systemctl stop adu-agent
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
3. Optional: If 'setfacl' command is available
   ```shell
   sudo setfacl -m "group::---" "/usr/lib/adu/adu-shell"
   sudo setfacl -m "user::r--" "/usr/lib/adu/adu-shell"
   sudo setfacl -m "user:adu:r-x" "/usr/lib/adu/adu-shell" 
    ```
