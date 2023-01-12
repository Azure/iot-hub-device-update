# Registering device update extensions

For the 5 extension types, here is the breakdown of registration multiplicity:

1. Content Handler - Multiple can be registered, each associated with updateType key
2. Update Manifest Handler - Multiple can be registered, with update manifest version key
3. Content Downloader - Exactly one can be registered
4. Download Handler - Multiple can be registered, each associated with `downloadHandlerId`
5. Component Enumerator - Exactly one can be registered

Note: Below `"sha256"` property values can be generated via:

```sh
openssl dgst -sha256 -binary /path/to/lib.so | openssl base64
```

## First-Party Extension Registrations

See `register_reference_extensions()` in the Debian package [postinst script](../../packages/debian/postinst)

## Registering Content Handler extension shared library

### Agent Command-line

```sh
    /usr/bin/AducIotAgent -l 2 --extension-type updateContentHandler --extension-id "microsoft/apt:1" --register-extension /path/to/libmicrosoft_apt_1.so
```

### Contents of resultant content_handler.json registration file

```sh
$ cat /var/lib/adu/extensions/update_content_handlers/microsoft_apt_1/content_handler.json
{
   "fileName":"/var/lib/adu/extensions/sources/libmicrosoft_apt_1.so",
   "sizeInBytes":2365288,
   "hashes": {
        "sha256":"/7IfVowZdcOYSNVqkrPHCa8aYBITg7gOj9/IIuX7CgU="
   },
   "handlerId":"microsoft/apt:1"
}
```

## Registering Update Manifest Handler extension shared library

### Agent Command-line to register update manifest handler

```sh
    $adu_bin_path -l 2 --extension-type updateContentHandler --extension-id "microsoft/update-manifest" --register-extension $adu_extensions_sources_dir/$adu_steps_handler_file
    $adu_bin_path -l 2 --extension-type updateContentHandler --extension-id "microsoft/update-manifest:4" --register-extension $adu_extensions_sources_dir/$adu_steps_handler_file
    $adu_bin_path -l 2 --extension-type updateContentHandler --extension-id "microsoft/update-manifest:5" --register-extension $adu_extensions_sources_dir/$adu_steps_handler_file
```

### Contents of resultant content_handler.json for update manifest handler

```sh
$ cat /var/lib/adu/extensions/update_content_handlers/microsoft_update-manifest_5/content_handler.json
{
   "fileName":"/var/lib/adu/extensions/sources/libmicrosoft_steps_1.so",
   "sizeInBytes":2151872,
   "hashes": {
        "sha256":"PV13dnM1uOzzYguZl/jq27i/xv+HXwPauvYrJBsNXvI="
   },
   "handlerId":"microsoft/update-manifest:5"
}
```

## Registering Content Downloader extension shared library

### Agent Command-line to register content downloader

```sh

    $adu_bin_path -l 2 --extension-type contentDownloader --register-extension $adu_extensions_sources_dir/$adu_delivery_optimization_downloader_file
```

### Contents of resultant extension.json for content downloader

```sh
cat /var/lib/adu/extensions/content_downloader/extension.json
{
   "fileName":"/var/lib/adu/extensions/sources/libdeliveryoptimization_content_downloader.so",
   "sizeInBytes":254584,
   "hashes": {
        "sha256":"0amRRIkSZ/im/AoLahg7QZwzZWo837VPEa1zedXl9BA="
   }
}
```

## Registering Download Handler extension shared library

### Agent Command-line to register download handler

```sh
    $adu_bin_path -l 2 --extension-type downloadHandler --extension-id "microsoft/delta:1" --register-extension $adu_extensions_sources_dir/$adu_delta_download_handler_file
```

### Contents of resultant extension.json for download handler

```sh
$ cat /var/lib/adu/extensions/download_handlers/microsoft_delta_1/download_handler.json
{
   "fileName":"/var/lib/adu/extensions/sources/libmicrosoft_delta_download_handler.so",
   "sizeInBytes":1774008,
   "hashes": {
        "sha256":"ldzJKSkUCjG7YdXxDbAFk/LwLPsmDzvXLwCZyGwnzBQ="
   },
   "handlerId":"microsoft/delta:1"
}
```

## Registering Component Enumerator extension shared library

### Agent Command-line to register component enumerator

```sh
$adu_bin_path -l 2 --extension-type componentEnumerator --register-extension $adu_extensions_sources_dir/$adu_example_component_enumerator_file
```

### Contents of extension.json for component enumerator

```sh
cat /var/lib/adu/extensions/component_enumerator/extension.json
{
   "fileName":"/var/lib/adu/extensions/sources/libcontoso_component_enumerator.so",
   "sizeInBytes":117896,
   "hashes": {
        "sha256":"z+X29flqe4MLbkDm5i2xWJAVJSyQTLamMZOhiszmw9k="
   }
}
```
