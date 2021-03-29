# Multi Component Update Script Fiels

## Type Of Scripts

### Device Pre-Install Script
- Compatibility checks
- Prepare device for the updates

### Device Post-Install Script
- Update validation
- Cleanup all unuse resources

### Component Pre-Install Script
- Component compatibility checks
- Prepare device and component for the update

### Component Post-Install Script
- Component update validation
- Cleanup all unused resources

### Script Arguments
Argument | Type | Description |
|----|----|----|
|`-c`, `--cid`, or `--component-id`| string | A component ID.<br>***Note:** only required for component's pre-install and post-install scripts.*
|`-l`, or `--log-file`| string | A full path to the log file for this script.<br><br>File name format is :<br>[path]/[workflowid]-[datetime]-[scriptname].log |

If specified, additional `arguments` property can be specified. This string will be passed to the script.  

For example:  

```text
"preInstall" : {  
        "fileName" : "smart-vacuum-main-script.sh", 
        "sizeInBytes" : 1234,  
        "hashes" : {  
            "sha256" : "trVvzUg6f+3CxI6UVtEFyp3BukmtsFOmu5lSPGs0THE="  
        }, 
        "arguments" : "--task preInstall --scope device", 
        "downloadOnDemand" : false 
 }, 
```

### Script Exit Code and Output

#### Pre-Install Exit Code

Exit Code| Reason | Comment
----|----|----
0| Succeeded | Pre-Install script completed successfully.<br>The Agent should proceed with the component update.
1| Failure | General errors. The update
3| Failure | The **device** is not ready
4| Failure | The **component** is not ready
5| Failure | Update is incompatible with the targget.

> **NOTE:** When failed, reports exit code, and error message "script_xyz.sh failed".  

> **NOTE:** These error codes are under a pending Design Review
>  
> [Open Issue]  How to communicate to the agent that the update can be skipped? (e.g., already installed, an optional target is not available, etc.)

#### Post-Install Exit Code

Exit Code| Reason | Comment
----|----|----
0| Succeeded | Post-Install script completed successfully.<br>The Agent should proceeded with the next step.
1| Failure | General errors. The update
6| Failure | A fatal clean up error occured.
7| Failure | A target component is in a bad state.<br>e.g., *The target module cannot be start.*
8| Failure | Post-install validation failed.

> **NOTE:** When failed, reports exit code, and error message "script_xyz.sh failed".  

> **NOTE:** These error codes are under a pending Design Review
>  
> [Open Issue]  How to communicate to the agent that the update can be skipped? (e.g., already installed, an optional target is not available, etc.)
