# Configuration APIs
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

"enabledIotHubSDKTracing" : true,

"logProvider": {
    "path" : "/usr/lib/adu/providers/zlog.so",
    "logProviderDAta" : {
        "logLevel" : 0
    }
},

"agentTrustedUsers": ["adu", "do"],

"agentTrustedGroups" : ["adu", "do"],

"agents":[
  {
      "name":"host-update",
      "connectionInfoProvider":"AIS",
      "connectionData":{
          "connectionType" : "module",
          "moduleName" : "IoTDeviceUpdate"
      },

      "aduc_manufacturer" : "Contoso",
      "aduc_model" : "Smart-Box"
  },

  {
      "name":"leaf-update",
      "connectionInfoProvider":"string",
      "connectionData":{
          "connectionString" : "HostName=..."
      },

      "aduc_manufacturer" : "Fabricam",
      "aduc_model" : "Camera"
  }
],

"updateHandlers":[
    {
        "updateType":"microsoft/apt:1",
        "path":"/user/lib/adu/handlers.d/microsoft-apt-1/microsoft-apt-1.so",
        "sha256":"xAbsdf802x3233="
    },
    {
        "updateType":"microsoft/swupdate:1",
        "path":"/user/lib/adu/handlers.d/microsoft-swupdate-1/microsoft-swupdate-1.so",
        "sha256":"xAbsdfsd23rs302x3123="
    },
    {
        "updateType":"microsoft/mcu:1",
        "path":"/user/lib/adu/handlers.d/microsoft-mcu-1/microsoft-mcu-1.so",
        "sha256":"xAbsdf802x3233="
    },
]
```