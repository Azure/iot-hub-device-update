# APT Update Handler

The **APT Update Handler** enables the device to support the **package-based** updates.

Package-based updates are targeted updates that alter only a specific component or application on the device. They use an APT manifest which provides the Device Update Agent with the information it needs to download and install the packages specified in the APT Manifest file (as well as their dependencies) from a designated repository.

## APT Manifest

### Schema

````json
{
    "name": "<name>",
    "version": "<version>",
    "packages": [
        {
            "name": "<package name>",
            "version": "<version specifier>"
        }
    ]
}
````

### Example

````json
{
    "name":"tree-apt-manifest",
    "version":"1.0",
    "packages":[
        {
            "name":"tree",
            "version":"1.7.0-5"
        }
    ]
}
````

For more details, see [Device Update APT Manifest](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/device-update-apt-manifest)

More example APT manifest files can be found [here](../../../docs/tutorials)
