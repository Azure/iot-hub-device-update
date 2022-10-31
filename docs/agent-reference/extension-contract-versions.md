
# Extension Contract Versions

Every Extension Type shared library plugin includes a "GetContractInfo" function export symbol that explicitly opts into a particular version of the Agent<->ExtensionType extension contract, which includes:

- Function export symbols on the extension shared library that the Agent will call
- The expected function signature return types and function arguments for the particular contract version

## GetContractInfo export symbol

The `GetContractInfo` symbol is defined in [extension_common_export_symbols.h](../../src/extensions/inc/aduc/exports/extension_common_export_symbols.h) but the signature is documented separately for each extension type--look for `GetContractInfo` in:

- [content handler and update manifest handler exports](../../src/extensions/inc/aduc/exports/extension_content_handler_export_symbols.h)
- [content download exports](../../src/extensions/inc/aduc/exports/extension_content_downloader_export_symbols.h)
- [download handler exports](../../src/extensions/inc/aduc/exports/extension_download_handler_export_symbols.h)
- [component enumerator exports](../../src/extensions/inc/aduc/exports/extension_component_enumerator_export_symbols.h)

The contract versions follow semantic versioning(i.e. back compatibility break will increment the major version, else the minor version).

## Agent usage of GetContractInfo

The core agent will immediately lookup the `GetContractInfo` symbol (after loading the extension shared library [DSO](https://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html) with [dlopen(3)](https://linux.die.net/man/3/dlopen)) using [dlsym(3)](https://linux.die.net/man/3/dlsym).

If the symbol exists, it will be called to populate the ADUC_ExtensionContractInfo struct and it will store in memory the contract version on a per-extension-type manner for use in adjusting call patterns aligned with the contract version.

If the agent does not support the contract version opted-into by the extension, then it will fail with an ExtendedResultCode matching the pattern `ADUC_ERC_{ExtensionType}_UNSUPPORTED_CONTRACT_VERSION` in [result_codes.json](../../scripts/error_code_generator_defs/result_codes.json) or in the build-generated [src/inc/aduc/result.h](../../src/inc/aduc/result.h).

## Back compatibility

To support older custom extensions prior to GA, not including `GetContractInfo` symbol will be implicitly conflated with extension version 1.0 for the extension type, but eventually a version of the agent will make omitting `GetContractInfo` a failure, so it is strongly suggested to include it and to update older extensions.
