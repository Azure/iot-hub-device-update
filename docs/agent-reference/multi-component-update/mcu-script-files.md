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
|-c<br>--component| string | A component ID.<br>***Note:** only required for component's pre-install and post-install scripts.*
|-l<br>--log-file| string | A full path to the log file for this script |



### Script Exit Code and Output

This convention is apply to preInstall and postInstall scripts.

Exit Code| Reason | Comment
----|----|----
0| Success | Continue with the update
-1| Success | The update already installed on the device
-2| Success | The update is not required  
1-127 | Failed | When failed, reports exit code, and error message "script_xyz.sh failed".

> **NOTE:** These error codes are under a pending Design Review

### How MCU Update Handler Locates Files

The MCU Content Handler locates the file using `fileUri` property. ( See [File Information Property](./mcu-manifest.md#file-information-property) for detail )

- For script files, if the specified file does not exist, MCU Update Handler will try to download (if needed) and unpack the scripts bundle into the `sandbox` directory. Then, MCU Update Handler will search for the file again. If failed, MCU Update Handler will report `FILE_NOT_FOUND` error.

- For update files, e.g., `device firmware file`, `bootfs image file`, `rootfs image file`, or other `component's firmware image`, MCU Update Handler will search for the specified file in `ADU Update Manifest` `fileUrls` list. If found, MCU Update Handler will requests ADU Agent to download the file into the `sandbox` folder (via Delivery Optimization Agent)
