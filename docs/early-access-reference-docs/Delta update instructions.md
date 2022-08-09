
# Device Update for Azure IoT Hub

## How to use delta update in Early Access

### Summary

The Device Update for IoT Hub Azure offering is adding support for delta (differential) updates, targeting our General Availability release. Delta updates allow you to generate a very small update which represents only the changes between two full updates - a source image and a target image. This is ideal for reducing the bandwidth used to download an update to a device, particularly if there have only been a small number of changes between the source and target updates.

For our initial preview release of delta functionality, there are some specific constraints:

1.	The source and target updates must:  
a.	Be .swu format (SWUpdate)  
b.	Use Ext2, Ext3, or Ext4 filesystem  
c.	Be a raw image (writeable to device)  
d.	Compressed originally with gzip or zstd
2.	The delta generation process will re-compress the target .swu update using ZSTD compression in order to produce an optimal delta. You'll import this re-compressed target .swu update to the DU service along with the generated delta update file.
3.	ZSTD decompression must be enabled in SWUpdate on the device.  
a.	Note: this requires using [SWUpdate 2019.11](https://github.com/sbabic/swupdate/releases/tag/2019.11) or later.

### Download all needed files

In order to use the Device Update delta update preview, you will need some files. Please download all the files in **iot-hub-device-update/tools/delta/**, as we will reference those items at various points in the instructions below.

### Configure a device with Device Update Agent and delta processor component

**Device Update Agent**

To add the Early Access Device Update Agent to a device and configure it for use, use the latest Early Access version of the Agent and follow the instructions from our public documentation: [Provisioning Device Update for Azure IoT Hub Agent](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/device-update-agent-provisioning). Note that you will also need to include an SWUpdate [Update Handler](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/device-update-agent-overview#update-handlers) which integrates with the DU Agent to perform the actual update install. We have included a Debian 11 build script as an example - the key takeaway is to enable ZSTD decompression in SWUpdate!

**Delta processor**

You will find all the Delta processor code in the file you previously downloaded: **Delta_processor.zip**

To add the delta processor component to your device image and configure it for use, use apt-get to install the proper Debian package for your platform (it should be named ms-adu_diffs_1.0.3_amd64.deb for amd64):  
`sudo apt-get install [path to Debian package]`

Alternatively, on a non-Debian Linux you can install the shared object (libadudiffapi.so) directly by copying it to the /usr/lib:  
`sudo cp [path to libadudiffapi.so] /usr/lib/libadudiffapi.so`  
`sudo ldconfig`

### Deploy a full image update to your device

After a delta update has been downloaded to a device, in order to be re-created into a full image, it must be diffed against a valid _source .swu file_ which has been previously cached on the device. For this preview, the simplest way to populate this cached image is to deploy a full image update to the device via the DU service (using the existing import and deployment processes in our [public documentation](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/)). As long as the device has been configured with the Early Access DU Agent and Delta processor, the installed .swu file will be cached automatically by the DU Agent for future delta update use.

### Generate delta updates using the DiffGen tool

**Environment Pre-Requisites**  
Before creating deltas with DiffGen, several things need to be downloaded and/or installed on the environment machine. We recommend a Linux environment and specifically Ubuntu 20.04  (or WSL if natively on Windows).

The following table provides a list of the content needed, where to retrieve them and the recommended installation if required.

---

| Binary Name       | Where to acquire      | How to install        |
|-------------------------------|----------------------------------------------------------------------------|------------------------------------|
| DiffGen           | You will find all the DiffGen code in the file you previously downloaded: **Delta_generation.zip**  | Download all content and place into a known directory.
| .NET (Runtime)    |Via Terminal / Package Managers    | Since running a pre-built version of the tool, only the Runtime is required. [Microsoft Doc Link](https://docs.microsoft.com/en-us/dotnet/core/install/linux-ubuntu).

---

**Dependencies**

The zstd_compression_tool is used for decompressing an archive's image files and recompressing them with zstd. This ensures all archive files used for diff generation have the same compression algorithm for the images inside the archives.

Commands to install required packages/libraries:  
`sudo apt update`  
`sudo apt-get install -y python3 python3-pip`  
`sudo pip3 install libconf zstandard`

**How to run DiffGen**  

The DiffGen tool is run with several arguments. All arguments are required, and overall syntax looks like the following:

`DiffGenTool [source_archive] [target_archive] [output_path] [log_folder] [working_folder] [recompressed_target_archive]`  
- The script recompress_tool.py will be run to create the file [recompressed_target_archive], which will then be used instead of [target_archive] as the target file for creating the diff
- The image files within [recompressed_target_archive] will be compressed with ZSTD

If your .swu files are signed (likely), you will need an additional argument as well:

`DiffGenTool [source_archive] [target_archive] [output_path] [log_folder] [working_folder] [recompressed_target_archive] "[signing_command]"`
- In addition to using [recompressed_target_archive] as the target file, providing a signing command string parameter will run recompress_and_sign_tool.py to create the file [recompressed_target_archive] and have the sw-description file within the archive signed (i.e. sw-description.sig file will be present)

The following table describes the arguments in more detail:

---

| Argument  | Description   |  
|-------------------------------|----------------------------------------------------------------------------|  
| [source_archive]  | When creating the delta, the image that the delta will be based against. Important: this must be identical to the image that is already present on the device (e.g. cached from a previous update).|
| [target_archive]  | When creating the delta, the image that the delta will update the source image to.|
| [output_path] | The path (including the desired name of the delta file being generated) on the host machine where the delta will get placed after creation.  If the path does not exist, the directory will be created by the tool.|
| [log_folder]  | The path on the host machine where logs will get created and dropped into. We recommend defining this location as a sub folder of the output path. If path does not exist, it will be created by the tool. |
| [working_folder]  | Path on the machine where collateral and other working files are placed during the delta generation. We recommend defining this location as a subfolder of the output path. If the path does not exist, it will be created by the tool. |
| [recompressed_target_archive]  | The path on the host machine where the recompressed target file will be created. This file will be used instead of <target_archive> as the target file for diff generation. If this path exists before calling DiffGenTool, the path will be overwritten. We recommend defining this path as a file in the subfolder of the output path. |
| "[signing_command]" _(optional)_    | The desired command used for signing the sw-description file within the recompressed archive file.  
NOTE: Surrounding the parameter in double quotes is needed so that the whole command is passed in as a single parameter.  
NOTE: [recompressed_target_archive] must also be provided if using this parameter.  
NOTE: DO NOT put the '~' character in a key path used for signing, use the full home path instead. For example, use /home/USER/keys/priv.pem instead of ~/keys/priv.pem |

---

### DiffGen Examples
In the examples below, we are operating out of the /mnt/o/temp directory (in WSL):

_Creating diff between input source file and recompressed target file:_  
`sudo ./DiffGenTool`  
`/mnt/o/temp/[source file.swu]`  
`/mnt/o/temp/[target file.swu]`  
`/mnt/o/temp/[delta file to be created]`  
`/mnt/o/temp/logs`  
`/mnt/o/temp/working`  
`/mnt/o/temp/[recompressed file to be created.swu]`

_Creating diff between input source file and recompressed/re-signed target file_  
`sudo ./DiffGenTool`  
`/mnt/o/temp/[source file.swu]`  
`/mnt/o/temp/[delta file to be created]`  
`/mnt/o/temp/logs`  
`/mnt/o/temp/working`  
`/mnt/o/temp/[recompressed file to be created.swu]`  
`"openssl dgst -sha256 -sign [path to directory with key]/priv.pem"`

Note: if you encounter an error "_Parameters failed. Status: MissingBinaries Issues: dumpextfs zstd_compress_file bsdiff_", add executable permissions for those files (e.g. set chmod 755).

## Importing the generated delta update into Device Update for IoT Hub

### Generate import manifest

The basic process of importing an update to the Device Update service is unchanged from our public documentation, so if you haven't already, be sure to review this page:

[How to prepare an update to be imported into Azure Device Update for IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/create-update)

Importantly, however, there are specific aspects of delta support which are not fully implemented yet for this preview. Therefore, we have created a script to simplify the process during Early Access, which you previously downloaded: **New-ImportManifest.ps1**. Note: the script uses PowerShell, which can be [installed](https://docs.microsoft.com/en-us/powershell/scripting/install/installing-powershell) on Linux, Windows, or MacOS.

The first step in importing an update into the Device Update service is always to create an import manifest. You can learn about the import manifest concept [here](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/import-concepts#import-manifest), but note that delta updates require a new import manifest format that is not yet ready for our public documentation. Therefore, please use New-ImportManifest.ps1 instead to generate your import manifest.

The script includes example usage. The new/unique elements for delta update relative to our publicly-documented import manifest format are "-DeltaFile" and "-SourceFile", and there is a specific usage for the "-File" element as well:
 - The **File** element represents the Target update used when generating the delta.
 - The **SourceFile** element represents the Source update used when generating the delta. 
    - Of note, this is also the update version that must be available on the device.
- The **DeltaFile** element represents the generated delta (the output of the DiffGen tool when used with the appropriate Source and Target updates).

### Import using the Azure portal UI

To import the delta update, follow the public documentation here: [Add an update to Device Update for IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/import-update#import-an-update)

Note that you must include the following when importing:
- The import manifest .json file you created in the previous step.
- The recompressed target .swu image created when you ran the DiffGen tool previously.
- The delta file created when you ran the DiffGen tool previously.

### Deploying the delta update to your devices

When you deploy a delta update, the experience in the Azure portal will look identical to deploying a regular image update. For more information on deploying updates, see this public documentation page: [Deploy an update by using Device Update for Azure IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/deploy-update)

Once you have created the deployment for your delta update, the Device Update service and client will automatically identify if there is a valid delta update for each device you are deploying to. If a valid delta is found, the delta update will be downloaded and installed on that device. If there is no valid delta update found, the full image update (the Target .swu update) will be downloaded instead. This ensures that all devices you are deploying the update to will get to the appropriate version.

There are three possible outcomes for a delta update deployment:
- Delta update installed successfully. Device is on new version.
- Delta update failed to install, but a successful fallback install of the full image occurred instead. Device is on new version.
- Both delta and fallback to full image failed. Device is still on old version.


To determine which of the above outcomes occurred, you can view the Device Twin lastInstallResult section:

`"lastInstallResult": {`  
    `"resultCode": [see below],`  
    `"extendedResultCode": [see below],`


If the delta update succeeded:
- resultCode: [value greater than 0]
- extendedResultCode: 0

If the delta update failed but did a successful fallback to full image:
- resultCode: [value greater than 0]
- extendedResultCode: [non-zero; will be further defined by GA]

If the update was unsuccessful:
 - Start with the Device Update Agent errors in **error_code_markdown.pdf**, which you previously downloaded. 
    - Errors from the Device Update Agent that are specific to the Download Handler functionality used for delta updates begin with 0x9:

---

|Component  |Decimal    |Hex    |Note   |
|-------------------------------|----------------------------------------------------------------------------|----|----|  
| EXTENSION_MANAGER   |0  |0x00   |Indicates errors from extension manager download handler logic.  e.g. 0x900XXXXX
| PLUGIN    |1  |0x01   |Indicates errors with usage of download handler plugin shared libraries.  e.g. 0x901XXXXX
| RESERVED	|2 - 7  |0x02 - 0x07    |Reserved for Download handler.   e.g. 0x902XXXXX
| COMMON    |8  |0x08   |Indicates errors in Delta Download Handler extension top-level logic.   e.g. 0x908XXXXX
| SOURCE_UPDATE_CACHE   |9  |0x09   |Indicates errors in Delta Download handler extension Source Update Cache.   e.g. 0x909XXXXX
| DELTA_PROCESSOR   |10 |0x0A   |Error code for errors from delta processor API.   e.g. 0x90AXXXXX
 
- If the error code is not present in the PDF, it is likely an error in the delta processor component (separate from the DU Agent). If so, the extendedResultCode will be a negative decimal value of the following hexadecimal format: 0x90AXXXXX
    - 9 is "Delta Facility"
    - 0A is "Delta Processor Component" (aka ADUC_COMPONENT_DELTA_DOWNLOAD_HANDLER_DELTA_PROCESSOR)
    - XXXXX is the 20-bit error code from FIT delta processor
- If you are not able to solve the issue based on the error code information, please file a GitHub issue so we can address it. Thanks!
