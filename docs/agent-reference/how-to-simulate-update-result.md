# How To Simulate Update Results

You can use the Simulator Update Handler to simulate the Device Update workflow without actually downloading and installing any files to the actual device for demonstration or testing purposes.  

The source code for this Simulator Update Handler can be found at [src/content_handlers/simulator_handler](../../src/content_handlers/simulator_handler)

## Register Simulator Update Handler

To register the Simulator Update Handle, use following command:

```sh
sudo /usr/bin/AducIotAgent --update-type <update type> --register-content-handler <full path to the simulator update handler>
```

Below is an example of how to register Simulator Update Handler for handle 'microsoft/apt:1' update:

```sh
sudo /usr/bin/AducIotAgent --update-type 'microsoft/apt:1' --register-content-handler /var/lib/adu/extensions/sources/libmicrosoft_simulator_1.so
```

## Create Simulator Data File

The Simulator Update Handler by default will returns following results:

| Function | Default Result |
|---|---|
| Download | { "resultCode" : 500 } |
| Install | { "resultCode" : 600 } |
| Apply | { "resultCode" : 700 } |
| Cancel | { "resultCode" : 800 } |
| IsInstalled | { "resultCode" : 900 } |

**Note:** See `ADUC_ResultCode` enum in [aduc_core.h](../../src/adu_types/inc/aduc/types/adu_core.h)  

To override one or more default result above, the desired result can be specified in the Simulator Data file (**du-simulator-data.json**).  

The Simulator Update Handler will search for this file in `temp` directories specified by following system environment variables (in order):

```c
TMPDIR
TMP
TEMP
TEMPDIR
```

If none of the above system environment variables are specified, the file must be place in `/tmp` directory (**/tmp/du-simulator-data.json**)

You can modify the Simulator Update Handler source code to specify your own search preference. See [simulator_handler.cpp](../../src/content_handlers/simulator_handler/src/simulator_handler.cpp)

### Simulate Data File Schema

```json
{
    "version" : "1.0",
    "download" : {
            "<file name #1>" : {
                "resultCode" : <desired result code>, 
                "extendedResultCode" : <desired extended result code>,
                "resultDetails" : "<desired result details string>"
            }
            "<file name #2>" : {
                "resultCode" : <desired result code>, 
                "extendedResultCode" : <desired extended result code>,
                "resultDetails" : "<desired result details string>"
            },
            ...
            "<file name #N>" : {
                "resultCode" : <desired result code>, 
                "extendedResultCode" : <desired extended result code>,
                "resultDetails" : "<desired result details string>"
            },        
            "*" : { // A fall back result for all unmatched file names
                "resultCode" : <desired result code>, 
                "extendedResultCode" : <desired extended result code>,
                "resultDetails" : "<desired result details string>"
            }
        },
    "install" : {
        "resultCode" : <desired result code>, 
        "extendedResultCode" : <desired extended result code>,
        "resultDetails" : "<desired result details string>"
    },
    "apply" : {
        "resultCode" : <desired result code>, 
        "extendedResultCode" : <desired extended result code>,
        "resultDetails" : "<desired result details string>"
    },
    "cancel" : {
        "resultCode" : <desired result code>, 
        "extendedResultCode" : <desired extended result code>,
        "resultDetails" : "<desired result details string>"
    },
    "isInstalled" : {
        "<installed criteria string #1>" : {
            "resultCode" : <desired result code>, 
            "extendedResultCode" : <desired extended result code>,
            "resultDetails" : "<desired result details string>"
        },
        "<installed criteria string #2>" : {
            "resultCode" : <desired result code>, 
            "extendedResultCode" : <desired extended result code>,
            "resultDetails" : "<desired result details string>"
        },
        ...
        "<installed criteria string #N>" : {
            "resultCode" : <desired result code>, 
            "extendedResultCode" : <desired extended result code>,
            "resultDetails" : "<desired result details string>"
        },
        "*" : { // A fall back result for all unmatched 'installedCriteria' string
            "resultCode" : <desired result code>, 
            "extendedResultCode" : <desired extended result code>,
            "resultDetails" : "<desired result details string>"
        }
    }
}
```

### Simulate 'Download' Action Result

To simulate the desired result for 'download' action, place the following JSON data in the Simulator Data file:

```json
    "download" : {
        "<file name #1>" : {
            "resultCode" : <desired result code>, 
            "extendedResultCode" : <desired extended result code>,
            "resultDetails" : "<desired result details string>"
        }
        "<file name #2>" : {
            "resultCode" : <desired result code>, 
            "extendedResultCode" : <desired extended result code>,
            "resultDetails" : "<desired result details string>"
        },
        ...
        "<file name #N>" : {
            "resultCode" : <desired result code>, 
            "extendedResultCode" : <desired extended result code>,
            "resultDetails" : "<desired result details string>"
        },        
        "*" : { // A fall back result for all unmatched file names
            "resultCode" : <desired result code>, 
            "extendedResultCode" : <desired extended result code>,
            "resultDetails" : "<desired result details string>"
        }
    }
```

**NOTE:** The map keys above must match the `file name` specified in the `Update Manifest` that the Device Update Agent received.

The **"*"** name indicates a fallback value for all un-matched file names.

The following is an example of how to simulate all download succeeded:

```json
    "download" : {
        "*" : {  // A fall back result for all unmatched file names
            "resultCode" : 600,         // ADUC_Result_Download_Success
            "extendedResultCode" : 0,   // No extended result
            "resultDetails" : ""
        }
    }
```

### Simulate 'Install' Action Result

You can specify only one result for the 'install' action. To simulate the desired result for the 'install' action, place the following JSON data in the Simulator Data file:

```json
    "install" : {
        "resultCode" : <desired result code>, 
        "extendedResultCode" : <desired extended result code>,
        "resultDetails" : "<desired result details string>"
    }
```

Following is an example of how to simulate an install error caused by missing file:

```json
    "install" : {
        "resultCode" : 0,                 // ADUC_Result_Failure
        "extendedResultCode" : 805306670, // 0x3000012E - ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_MISSING_PRIMARY_FILE
        "resultDetails" : "Simulating error caused by missing file."
    }
```

### Simulate 'Apply' Action Result

You can specify only one result for the 'apply' action. To simulate the desired result for the 'apply' action, place the following JSON data in the Simulator Data file:

```json
    "apply" : {
        "resultCode" : <desired result code>, 
        "extendedResultCode" : <desired extended result code>,
        "resultDetails" : "<desired result details string>"
    }
```

### Simulate 'Cancel' Action Result

You can specify only one result for the 'cancel' action. To simulate the desired result for the 'cancel' action, place the following JSON data in the Simulator Data file:

```json
    "cancel" : {
        "resultCode" : <desired result code>, 
        "extendedResultCode" : <desired extended result code>,
        "resultDetails" : "<desired result details string>"
    }
```

### Simulate 'IsInstalled' Check Result

To simulate the desired result for the 'isInstalled' check, place the following JSON data in the Simulator Data file:

```json
    "isInstalled" : {
        "<installed criteria string #1>" : {
            "resultCode" : <desired result code>, 
            "extendedResultCode" : <desired extended result code>,
            "resultDetails" : "<desired result details string>"
        },
        "<installed criteria string #2>" : {
            "resultCode" : <desired result code>, 
            "extendedResultCode" : <desired extended result code>,
            "resultDetails" : "<desired result details string>"
        },
        ...
        "<installed criteria string #N>" : {
            "resultCode" : <desired result code>, 
            "extendedResultCode" : <desired extended result code>,
            "resultDetails" : "<desired result details string>"
        },
        "*" : { // A fall back result for all unmatched 'installedCriteria' string
            "resultCode" : <desired result code>, 
            "extendedResultCode" : <desired extended result code>,
            "resultDetails" : "<desired result details string>"
        }
    }
```

You can specify multiple results for 'isInstalled' check, for example:

```json
    "isInstalled" : {
        "1.0" : {
            "resultCode" : 900,         // ADUC_Result_IsInstalled_Installed 
            "extendedResultCode" : 0,
            "resultDetails" : ""
        },

        "1.2" : {
            "resultCode" : 900,         // ADUC_Result_IsInstalled_Installed 
            "extendedResultCode" : 0,
            "resultDetails" : ""
        },

        "*" : {                         // Indicates a fall back result for all unmatched 'installedCriteria' string
            "resultCode" : 901,         // ADUC_Result_IsInstalled_NotInstalled 
            "extendedResultCode" : 0,
            "resultDetails" : "This update is not installed."
        }
    }
```

**NOTE:** The map keys above must match the `installedCriteria` string specified in the `Update Manifest` that the Device Update Agent received.
