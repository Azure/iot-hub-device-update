# Download Handler Extensibility Point

A download handler extension is a `.so` that exports the function symbols + signatures defined in [extension_download_handler_export_symbols.h](../inc/aduc/exports/extension_download_handler_export_symbols.h).

When the [v5 update manifest](../../../docs/agent-reference/update-manifest-v5-schema.md#what-changed-between-v4-and-v5) has a `downloadHandler.id` property on an update payload file, the core agent checks the on-disk download-handler registry for a `.so` registered under that id. If one is found, the agent loads it and calls the following exports during the **Download** phase:

| Export | When called | Required? |
|---|---|---|
| `Initialize(logLevel)` | At plugin load (plugin ctor, `download_handler_plugin.cpp:34-39`) | Recommended |
| `GetContractInfo(out)` | Before first `ProcessUpdate` (`extension_manager_helper.cpp:71`) | **Required** — contract version is enforced |
| `ProcessUpdate(workflow, fileEntity, targetPath)` | For each file with a matching `downloadHandler.id` (`extension_manager_helper.cpp:106`) | **Required** |
| `OnUpdateWorkflowCompleted(workflow)` | After a successful Apply (`agent_workflow.c:1257`) | Recommended — handlers that need to retain payloads on success use this |
| `CacheSourceUpdate(workflow)` | Immediately before reboot/agent-restart triggered by Apply (`agent_workflow.c:1750, 1798`) | Optional — `download_handler_plugin.cpp:181-196` returns success when the export is absent |
| `Cleanup()` | At plugin unload (plugin dtor) | Recommended |

### `ProcessUpdate` return contract

`ProcessUpdate` returns `ADUC_Result`. The agent reacts to three buckets (`extension_manager.cpp:973-1011`):

- `ADUC_Result_Download_Handler_SuccessSkipDownload` — handler produced the target payload at `targetFilePath`; agent **skips** the standard download for this file.
- `ADUC_Result_Download_Handler_RequiredFullDownload` — **success-bucket** code meaning "I cannot produce this payload, please download it normally"; agent **falls back** to the standard content downloader.
- Any failure code — agent records the ERC against the workflow and **falls back** to the standard content downloader (same path as `RequiredFullDownload`).

In all cases the agent re-verifies the final payload's SHA-256 against `files.<id>.hashes` from the signed manifest before continuing to Install, so the trust chain is preserved regardless of how the file was obtained.

## Reference implementation: Microsoft Delta Download Handler

The [Microsoft Delta Download Handler](./plugin_examples/microsoft_delta_download_handler/handler/plugin/src/microsoft_delta_download_handler_plugin.EXPORTS.c) registers the id `microsoft/delta:1`. It uses the file's `relatedFiles` map to advertise candidate delta payloads (one per known source version). For each candidate it:

1. Reads `microsoft.sourceFileHash` + `microsoft.sourceFileHashAlgorithm` from `relatedFiles.<rid>.properties`.
2. Looks up the source full payload in the on-disk source-update cache by `(target update provider, alg, hash)`. The default cache root is **`/var/lib/adu/sdc`** (set in `CMakeLists.txt:517-522`).
3. On cache hit: downloads only the small `.diff` file (the agent's content downloader verifies its hash), then calls `libadudiffapi` to apply the diff against the cached source and write the reconstructed target into the sandbox. Returns `SuccessSkipDownload`.
4. On cache miss for every candidate: returns `RequiredFullDownload` so the agent downloads the full payload normally; that full payload is then cached on workflow success so it can serve as a source for future deltas.

> **For the full runtime walkthrough, sequence diagram, source-cache layout, trust-chain analysis, and a worked manifest example, see [docs/agent-reference/delta-download-handler.md](../../../docs/agent-reference/delta-download-handler.md).** For build/install instructions see [building-with-delta-handler.md](../../../docs/agent-reference/building-with-delta-handler.md).


> **For the full runtime walkthrough, sequence diagram, source-cache layout, trust-chain analysis, and a worked manifest example, see [docs/agent-reference/delta-download-handler.md](../../../docs/agent-reference/delta-download-handler.md).** For build/install instructions see [building-with-delta-handler.md](../../../docs/agent-reference/building-with-delta-handler.md).

## Producing deltas (delta-generation toolchain)

The diff file is produced offline (typically as part of your update-import pipeline) using the [Diff Generation tool](https://github.com/Azure/iot-hub-device-update-delta#diff-generation) from the iot-hub-device-update-delta repo. The tool emits two artifacts:

- **The diff file** (e.g. `v1-to-v2.diff`) — binary patch, typically 5-20% of the full update size.
- **A recompressed target `.swu`** — the target SWUpdate payload with its inner ext3/ext4 raw image recompressed with `zstd`. Both the source and target SWUs must use the same recompression settings — otherwise diff application cannot reproduce a byte-identical target and the final SHA-256 verification in the agent will fail.

### SWUpdate requirements on the device

The on-device `swupdate` binary must be built with `CONFIG_ZSTD=y`. Zstd support landed in [swupdate 2019.11](https://github.com/sbabic/swupdate/releases/tag/2019.11), so use that release or newer.

```bash
swupdate --help | grep -i zstd        # quick check
grep CONFIG_ZSTD /path/to/swupdate/.config   # build-config check
```

### Recommended step-handler pairing

Pair the delta handler with [`microsoft/swupdate:2`](../update_manifest_handlers/steps_handler/README.md) so the install step can invoke `swupdate` against the reconstructed (recompressed) target produced by the handler. Typical `handlerProperties`:

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

