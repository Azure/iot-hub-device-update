# Running the Device Update for IoT Hub Reference Agent

## Command Line

```bash
AducIotAgent [options...] '<IoT device connection string>' [simulator args...]
AducIotAgent [options...] -- [one or more of connection string and simulator or other additional args]
```

e.g.

```bash
AducIotAgent --enable-iothub-tracing --log-level 1 '<IoT device connection string>'
AducIotAgent --enable-iothub-tracing --log-level 1 -- '<IoT device connection string>'
```

#### Simulator examples

```bash
AducIotAgent '<IoT device connection string>' --simulation_mode=allsuccessful
AducIotAgent --log-level 0 -e '<Iot device connection string> --simulation_mode=downloadfailed
```

If providing additional args (e.g. simulator args) without a non-option (i.e. -c) connection string, then explicitly separate options from additional args using '--'.
e.g.

```bash
AducIotAgent -- --simulation_mode=allsuccessful '<IoT device connection string>' 
AducIotAgent -- --simulation_mode=allsuccessful
AducIotAgent --log-level 0 -e -- --simulation_mode=downloadfailed --device_manufacturer=MyContoso
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
