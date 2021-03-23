# Device Update Agent Configuration

## APIs

```c
// A config object handle.
typedef void* CONFIG_HANDLE

// Instancitate a config object from specified file.
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

"enabledIoTHubTracing" : true,
"logLevel" : 0,

"aduShellTrustedUsers": ["adu", "do"],

"agents":[
  {
      "name":"host-update",
      "runas":"adu",
      "connectionInfoSource":"AIS",
      "connectionData":{
          "connectionType" : "module",
          "moduleName" : "ioTHubDeviceUpdate"
      },

      "aduc_manufacturer" : "Contoso",
      "aduc_model" : "Smart-Box"
  },

  {
      "name":"leaf-update",
      "runas":"adu",
      "connectionInfoSource":"string",
      "connectionData":{
          "connectionString" : "HostName=..."
      },

      "aduc_manufacturer" : "Fabrikam",
      "aduc_model" : "Camera"
  }
],

"contentHandlers":[
    {
        "updateType":"microsoft/apt:1",
        "path":"/user/lib/adu/content-handlers.d/microsoft-apt-1/libmicrosoft-apt-1.so",
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
|enabledIoTHubTracing|boolean|Enable IoT Hub SDK tracing<br>Default is 'false'|
|logLevel|number|Set log verbosity<br>0 - debug<br>1 - info<br>2 - warning<br>3 - error|
|aduShellTrustedUsers|Array of string|List of users that allowed to launch adu-shell|
|agents|Array of [Agent Info](#agent-info)| List of child agent's information|



### Agent Info
Property Name | Type | Description
|----|----|----|
|name|string|Internal name of the agent.<br>This name will appears in logs.|
|runas|string|The user id for the child agent process|
|connectionInfoSource|string|Indicates this agent acquire a connection string to IoT Hub.<br>This can be `"string"` or `"AIS"`|
|connectionData|Json object|Contain data used by specified connection info source|
|aduc_manufacturer|string|Manufacturer name|
|aduc_model|string|Model name|
