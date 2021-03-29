# Device Update Agent Configuration

## APIs

```c
// A config object handle.
typedef void* CONFIG_HANDLE

// Instantiate a config object from specified file.
CONFIG_HANDLE LoadConfig(const char * configFilePath);

// Free all resources allocated for the config object.
void FreeConfig(CONFIG_HANDLE config);

// Returns value of the specified config name.
// (Output is a json string).
const char* GetValue(const char* name);

```

## Configuration File Format

See example `du-config.json` file format below:

```json

"enableIoTHubTracing" : true,
"logLevel" : 0,

"aduShellTrustedUsers": ["adu", "do"],

"agents":[
  {
      "name":"host-update",
      "runas":"adu",      
      "connectionSource" : {
          "connectionType" : "AIS",
          "connectionData" : "iotHubDeviceUpdate"
      },

      "aduc_manufacturer" : "Contoso",
      "aduc_model" : "Smart-Box"
  },

  {
      "name":"leaf-update",
      "runas":"adu",

      "connectionSource" : {
          "connectionType" : "string",
          "connectionData" : "HOSTNAME=..."
      },

      "aduc_manufacturer" : "Fabrikam",
      "aduc_model" : "Camera"
  }
],

"contentHandlers":[
    {
        "updateType":"microsoft/apt:1",
        "filePath":"/user/lib/adu/content-handlers.d/microsoft-apt-1/libmicrosoft-apt-1.so",

        "sha256":"xAbsdf802x3233="
    },
    {
        "updateType":"microsoft/swupdate:1",
        "path":"/user/lib/adu/content-handlers.d/microsoft-swupdate-1/libmicrosoft-swupdate-1.so",
        "sha256":"xAbsdfsd23rs302x3123="
    },
    {
        "updateType":"microsoft/mcu:1",
        "path":"/user/lib/adu/content-handlers.d/microsoft-mcu-1/libmicrosoft-mcu-1.so",
        "sha256":"xAbsdf802x3233="
    },
]
```

Property Name | Type | Description
|----|----|----|
|enableIoTHubTracing|boolean|(optional) Enable IoT Hub SDK tracing.<br>Default is 'false'|
|logLevel|number|(optional) Set log verbosity<br>0 - debug<br>1 - info (default)<br>2 - warning<br>3 - error|
|aduShellTrustedUsers|Array of string|List of users that allowed to launch adu-shell|
|agents|Array of [Agent Information](#agent-information)|List of child agent's information|

### Agent Information

Property Name | Type | Description
|----|----|----|
|name|string|Internal name of the agent.<br>This name will appears in logs.|
|runas|string|The user id for the child agent process|
|connectionSource|[Connection Source](#connection-source)|Indicates how this agent acquire a connection string to IoT Hub|
|aduc_manufacturer|string|Manufacturer name|
|aduc_model|string|Model name|

### Connection Source

Property Name | Type | Description
|----|----|----|
|connectionType|string|Indicates how the Agent accuire the connection string to IoT Hub.<br>This can be `"string"` or `"AIS"`|
|connectionData|string|Connection data.<br><br>If `connectionType` is `"string"`, the `connectionData` must be IoT Hub device connection string, or module connection string.<br><br>If `connectionType` is `"AIS"`, the `connectionData` must be a `module name`.<br><br>See [How To Configure AIS]() for more detail.