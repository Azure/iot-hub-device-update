# SWUpdate Handler Result Codes

The SWUpdate Handler v2 (`microsoft/swupdate:2`) defines its own extended result codes using the modular extension result code system.

## Files

| File | Purpose |
|------|---------|
| `swupdate_handler_result_codes.json` | Source JSON defining all result codes |
| `inc/aduc/swupdate_handler_result_codes.h` | Auto-generated C header (also copied to `src/inc/aduc/`) |

## Result Code Format

Extended Result Codes (ERC) are 32-bit values:

```
0xFCCRRRRR
  │││└─────── Result Code (20 bits)
  ││└──────── Component Code (8 bits)
  │└───────── Facility Code (4 bits)
```

For SWUpdate Handler:
- **Facility**: `0x3` (ADUC_FACILITY_EXTENSION_UPDATE_CONTENT_HANDLER)
- **Component**: `0x01` (ADUC_CONTENT_HANDLER_SWUPDATE)

## Adding New Result Codes

1. Edit `swupdate_handler_result_codes.json`
2. Add new entry:
   ```json
   {
       "name": "ADUC_ERC_SWUPDATE_HANDLER_YOUR_ERROR",
       "value": 1027,
       "description": "Description of your error"
   }
   ```
3. Reconfigure CMake (header regenerates automatically)
4. Commit both JSON and generated header

## Result Code Ranges

| Range | Purpose |
|-------|---------|
| 1-10 | General preparation errors |
| 32-33 | Configuration errors |
| 257-264 | Download errors |
| 510-519 | Install errors |
| 767-770 | Apply errors |
| 1024-1026 | Cancel errors |

## Documentation

See [Device Update Agent Extended Result Codes](../../../../docs/agent-reference/device-update-agent-extended-result-codes.md) for full documentation on the result code system.
