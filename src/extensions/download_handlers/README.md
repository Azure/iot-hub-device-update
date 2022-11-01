# Download Handler Extensibility Point

A download handler extension exports the function symbols + signatures defined in [extension_download_handler_export_symbols.h](../inc/aduc/exports/extension_download_handler_export_symbols.h)

When the [v5 update manifest](../../../docs/agent-reference/update-manifest-v5-schema.md) has a `downloadHandlerId` property on an update payload file, then the core agent will see if there exists a registered download handler for that id. If there is, it will load the associated shared library and lookup and call the following exports:

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

The diff file is produced using the [Diff Generation](https://github.com/Azure/iot-hub-device-update-delta#diff-generation) tool. This tool outputs two files:

1. The diff file
2. A new .swu update payload file with ext3/ext4 raw image in it that is recompressed with ZSTD compression algorithm.

The source .swu is baked into the OS image at the expected location for the cache, or will be placed there after either a first image-based update with that .swu or via a preceeding step (e.g. script handler) in a multi-step update.

It is recommended to use [swupdate handler v2 handler](../step_handlers/swupdate_handler_v2/README.md) (update type of "microsoft/swupdate:2") where: 
- `"scriptFileName"` in `"handlerProperties"` of import/update manifest is set to a payload file script that will call `swupdate` appropriately.
- `"swuFileName"` in `"handlerProperties"` is set to the payload file corresponding to the recompressed target .swu swupdate CPIO archive file.

`swupdate` executable will need to be built with `CONFIG_ZSTD=y` in swupdate's `.config` file.

NOTE: swupdate source code will need to be greater than equal to [release tag 2019.11](https://github.com/sbabic/swupdate/releases/tag/2019.11) since that was the release when zstd compression support was first added.
