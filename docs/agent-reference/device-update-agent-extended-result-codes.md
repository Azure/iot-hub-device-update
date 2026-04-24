# Device Update Agent Result Codes and Extended Result Codes

## Result Codes

Result Codes are visible in the Device or Module Twin:

| Result Code | C Macro                                             |
| ----------- | --------------------------------------------------- |
| 0           | ADUC_Result_Failure                                 |
| -1          | ADUC_Result_Failure_Cancelled                       |
| 500         | ADUC_Result_Download_Success                        |
| 501         | ADUC_Result_Download_InProgress                     |
| 502         | ADUC_Result_Download_Skipped_FileExists             |
| 503         | ADUC_Result_Download_Skipped_UpdateAlreadyInstalled |
| 504         | ADUC_Result_Download_Skipped_NoMatchingComponents   |
| 600         | ADUC_Result_Install_Success                         |
| 601         | ADUC_Result_Install_InProgress                      |
| 603         | ADUC_Result_Install_Skipped_UpdateAlreadyInstalled  |
| 604         | ADUC_Result_Install_Skipped_NoMatchingComponents    |
| 700         | ADUC_Result_Apply_Success                           |
| 800         | ADUC_Result_Cancel_Success                          |
| 801         | ADUC_Result_Cancel_UnableToCancel                   |
| 900         | ADUC_Result_IsInstalled_Installed                   |
| 901         | ADUC_Result_IsInstalled_NotInstalled                |
| 1000        | ADUC_Result_Backup_Success                          |
| 1001        | ADUC_Result_Backup_Success_Unsupported              |
| 1002        | ADUC_Result_Backup_InProgress                       |
| 1100        | ADUC_Result_Restore_Success                         |
| 1101        | ADUC_Result_Restore_Success_Unsupported             |
| 1102        | ADUC_Result_Restore_InProgress                      |

See [adu_core.h](../../src/adu_types/inc/aduc/types/adu_core.h) for more detail.

## Extended Result Code Structure (32 bits)

Extended Result Codes (ERCs) are 32-bit values structured as:

```
0xFCCRRRRR
  │││└─────── Result Code (20 bits, 0x00000 - 0xFFFFF)
  ││└──────── Component Code (8 bits, 0x00 - 0xFF)
  │└───────── Facility Code (4 bits, 0x0 - 0xF)
```

Facilities, components, and results are defined in [result_codes.json](../../scripts/error_code_generator_defs/result_codes.json). This JSON file is processed during the CMake configure step to generate [result.h](../../src/inc/aduc/result.h).

## Extension Result Codes

Step handlers can define their own result codes in separate JSON files within their extension directories. These are automatically included in `result.h` through the extension configuration system.

**Note:** Currently, only the **SWUpdate Handler v2** (`microsoft/swupdate:2`) uses this extension mechanism. Other step handlers may adopt this pattern in the future.

### SWUpdate Handler Result Codes

The SWUpdate handler defines its result codes in:
- **JSON Source:** `src/extensions/step_handlers/swupdate_handler_v2/swupdate_handler_result_codes.json`
- **Generated Header:** `src/inc/aduc/swupdate_handler_result_codes.h`

Example SWUpdate handler result code format:
- **Facility**: `0x3` (ADUC_FACILITY_EXTENSION_UPDATE_CONTENT_HANDLER)
- **Component**: `0x01` (ADUC_CONTENT_HANDLER_SWUPDATE)
- **Result**: Variable (defined in JSON)

### Extension Configuration

Extensions are registered in `scripts/error_code_generator_defs/extension_configs.json`:

```json
{
  "extensions": [
    {
      "name": "swupdate_handler_v2",
      "json_path": "../../src/extensions/step_handlers/swupdate_handler_v2/swupdate_handler_result_codes.json",
      "header_path": "swupdate_handler_result_codes.h",
      "description": "SWUpdate Handler v2 Extended Result Codes"
    }
  ]
}
```

## Result Code Generation System

### Generator Scripts

| Script | Purpose |
|--------|---------|
| `error_code_defs_generator.py` | Generates main `result.h` from `result_codes.json` |
| `extension_result_code_generator.py` | Generates extension-specific headers (e.g., `swupdate_handler_result_codes.h`) |
| `generate_all_extension_headers.py` | Wrapper that processes all extensions in `extension_configs.json` |

### Build Integration

The CMake build system automatically:
1. Runs `generate_all_extension_headers.py` to create extension headers
2. Runs `error_code_defs_generator.py` to create `result.h` with `#include` directives for extensions
3. Uses smart-write caching to avoid regeneration if sources haven't changed

### Adding New Result Codes to SWUpdate Handler

1. Edit `src/extensions/step_handlers/swupdate_handler_v2/swupdate_handler_result_codes.json`
2. Add new entry to `result_codes` array:
   ```json
   {
       "name": "ADUC_ERC_SWUPDATE_HANDLER_YOUR_ERROR",
       "value": 1027,
       "description": "Description of your error"
   }
   ```
3. Reconfigure CMake (headers regenerate automatically)
4. Commit both JSON and generated header

### Adding a New Extension (Future)

1. Create extension folder with `<extension>_result_codes.json`
2. Add entry to `scripts/error_code_generator_defs/extension_configs.json`
3. Reconfigure CMake to regenerate headers

## Decoding Error Codes

### Option 1: Search by Hex Value
Take the Extended Result Code (e.g., `0x30200002` or `807403522`) and search in:
- `src/inc/aduc/result.h`
- `src/inc/aduc/swupdate_handler_result_codes.h`

### Option 2: Manual Decode
Extract the facility, component, and result from the hex value and look up in the appropriate JSON file.

### Subprocess Error Codes

Some error codes come from subprocesses that don't record their codes in Device Update (e.g., Delivery Optimization, APT child processes).

- **Delivery Optimization**: Facility code `0xD` (13)
- **Extension common errors**: Result codes 0-300 with their facility

Search for `MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_*` macros in the codebase to find their origin.

## References

- [result.h](../../src/inc/aduc/result.h) - All extended result codes
- [result_codes.json](../../scripts/error_code_generator_defs/result_codes.json) - Core result code definitions
- [extension_configs.json](../../scripts/error_code_generator_defs/extension_configs.json) - Extension registry
- [swupdate_handler_result_codes.json](../../src/extensions/step_handlers/swupdate_handler_v2/swupdate_handler_result_codes.json) - SWUpdate handler result codes
