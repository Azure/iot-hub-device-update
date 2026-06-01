# Download Handler Extensibility Point

A download handler extension exports the function symbols + signatures defined in [extension_download_handler_export_symbols.h](../inc/aduc/exports/extension_download_handler_export_symbols.h)

When the [v5 update manifest](../../../docs/agent-reference/update-manifest-v5-schema.md#what-changed-between-v4-and-v5) has a `downloadHandlerId` property on an update payload file, then the core agent will see if there exists a registered download handler for that id. If there is, it will load the associated shared library and lookup and call the following exports:

- `GetContractInfo`
  - Populates the `contractInfo` out parameter
- `Initialize`
  - Initializes the handler
- `ProcessUpdate`
  - returns `ADUC_Result_Download_Handler_SuccessSkipDownload` if it is able to produce the update payload in the download sandbox work folder; otherwise, it returns a ResultCode of `ADUC_Result_DownloadHandler_RequiredFullDownload` or a failure ResultCode.

The Agent will not download the update payload if it receives `ADUC_Result_Download_Handler_SuccessSkipDownload` from `ProcessUpdate` call; otherwise, it will continue with a fallback download of the update payload as though the downloadHandlerId were not there.

After successful `Install` and `Apply`, the agent will call `OnUpdateWorkflowCompleted` symbol on the DownloadHandler.

When Agent shuts down, it will call `Cleanup()` on the DownloadHandler Extension.

## Usage by Microsoft Delta Download Handler to implement Delta Updates

The [Microsoft Delta Download Handler](./plugin_examples/microsoft_delta_download_handler/handler/plugin/src/microsoft_delta_download_handler_plugin.EXPORTS.c) is a DownloadHandler that calls the [Diff API](https://github.com/Azure/iot-hub-device-update-delta#diff-api) to produce a large swupdate .swu update payload from a "source .swu file" in a local [source update cache](./plugin_examples/microsoft_delta_download_handler/source_update_cache/inc/aduc/source_update_cache.h) and a much smaller diff file that is downloaded.

### Delta Update Flow Overview

```
┌─────────────────────────────────────────────────────────────────┐
│           Delta Download Handler Workflow                       │
└─────────────────────────────────────────────────────────────────┘

1. Update Manifest Processing
   └─> Agent detects "downloadHandlerId": "microsoft/delta:1"
   └─> Loads libmicrosoft_delta_download_handler.so
   └─> Calls ProcessUpdate() function

2. Source Cache Check
   └─> Handler examines "relatedFiles" in manifest
   └─> Searches for matching source in /var/lib/adu/cache/
   └─> Compares source file hash with manifest

3. Delta Download
   └─> If source found: Download only diff file (50MB)
   └─> If source not found: Try next relatedFile or fallback

4. Delta Reconstruction
   └─> Call libadudiffapi to reconstruct target from source + delta
   └─> (libadudiffapi uses bsdiff/bspatch algorithms internally)
   └─> Verify target hash matches manifest
   └─> Place target in download sandbox work folder

5. Return Result
   └─> Success: ADUC_Result_Download_Handler_SuccessSkipDownload
   └─> Agent skips downloading full file (saved 750MB!)
   └─> Failure: ADUC_Result_DownloadHandler_RequiredFullDownload
   └─> Agent proceeds with full download as fallback
```

### Delta Generation Process

The diff file is produced using the [Diff Generation](https://github.com/Azure/iot-hub-device-update-delta#diff-generation) tool. This tool outputs two files:

1. **The diff file** (`v1-to-v2.diff`)
   - Contains binary differences between source and target
   - Typically 5-20% of full update size
   - Generated using bsdiff algorithm

2. **A recompressed target .swu file** (`v2-recompressed.swu`)
   - Target update payload with ext3/ext4 raw image recompressed with ZSTD
   - Ensures consistent compression between source and target
   - Critical for reliable delta reconstruction

### Source Update Cache Management

The source .swu is placed in the cache via one of these methods:

1. **Baked into OS image** at build time (factory default)
   - Source SWU included in base image
   - Located at expected cache path: `/var/lib/adu/cache/`

2. **Cached after first full update**
   - First update is always full download
   - Handler caches recompressed version for future deltas

3. **Multi-step update with caching script**
   - Use script handler in preceding step
   - Script caches recompressed SWU before delta update step

### Handler Integration Requirements

It is recommended to use [swupdate handler v2 handler](../step_handlers/swupdate_handler_v2/README.md) (update type of "microsoft/swupdate:2") where:

- `"scriptFileName"` in `"handlerProperties"` of import/update manifest is set to a payload file script that will call `swupdate` appropriately and cache the recompressed file.

- `"swuFileName"` in `"handlerProperties"` is set to the payload file corresponding to the recompressed target .swu swupdate CPIO archive file.

Example handler properties:
```json
{
  "handler": "microsoft/swupdate:2",
  "files": ["update-v2.swu"],
  "handlerProperties": {
    "scriptFileName": "microsoft-delta-source-caching.sh",
    "swuFileName": "update-v2-recompressed.swu",
    "installedCriteria": "2.0.0"
  }
}
```

### SWUpdate Configuration Requirements

`swupdate` executable will need to be built with `CONFIG_ZSTD=y` in swupdate's `.config` file.

**NOTE:** swupdate source code will need to be greater than equal to [release tag 2019.11](https://github.com/sbabic/swupdate/releases/tag/2019.11) since that was the release when zstd compression support was first added.

**Verification:**
```bash
# Check if SWUpdate has zstd support
swupdate --help | grep -i zstd

# Or check build configuration
grep CONFIG_ZSTD /path/to/swupdate/.config
```

### Update Manifest Example with Delta

```json
{
  "updateId": {
    "provider": "Contoso",
    "name": "Device",
    "version": "2.0.0"
  },
  "files": {
    "update-v2.swu": {
      "filename": "update-v2-recompressed.swu",
      "sizeInBytes": 838860800,
      "hashes": {
        "sha256": "target_file_hash..."
      },
      "downloadHandlerId": "microsoft/delta:1",
      "relatedFiles": [
        {
          "filename": "v1-to-v2.diff",
          "sizeInBytes": 52428800,
          "hashes": {
            "sha256": "diff_file_hash..."
          },
          "properties": {
            "microsoft.sourceFileHashAlgorithm": "sha256",
            "microsoft.sourceFileHash": "source_v1_hash...",
            "microsoft.sourceVersion": "1.0.0"
          }
        }
      ]
    },
    "v1-to-v2.diff": {
      "filename": "v1-to-v2.diff",
      "sizeInBytes": 52428800,
      "hashes": {
        "sha256": "diff_file_hash..."
      }
    }
  }
}
```

### Bandwidth Savings Example

**Typical Scenario (800MB rootfs):**
- Full update: 800MB download
- Delta update: 50-80MB download (93-90% reduction)
- Time on 10Mbps: 10 minutes → 40-60 seconds

**Multiple Version Support:**
The handler can include multiple relatedFiles for different source versions:
- Device on v1.0 → Download v1-to-v3.diff (150MB)
- Device on v2.0 → Download v2-to-v3.diff (60MB)
- Device on v0.x → Download full v3.swu (800MB)

The handler automatically selects the optimal delta based on cached source availability.
