# Device Update Agent result codes and extended result codes

## Result Code

The followings are Result Code that visible in the Device or Module Twin:

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

See [adu_core.h](../../src/adu/types/adu_core.h) for more detail.

## Extended Result Code Structure (32 bits)

For each extended result code there is a facility which is made up of components which in turn have results. These facilities, components, and results are described in the [result_code.json](../../scripts/error_code_generator_defs/result_codes.json) file. The json file is processed when building with the `build.sh` script to generate the `result.h` file which is then used for compiling the project.

There is always a version of the `result.h` file checked-in to the GitHub repository [here](../../src/inc/aduc/result.h), however if there are changes made to the `result_codes.json` file you will need to re-generate the `result.h` file.

## Decoding an Error Code

To decode an error code within the repository you have two options.

1. Manually decrypt the facility, component, and error code and look it up in the result_codes.json
2. Take the whole Extended Result Code value in hexadecimal or decimal format (e.g. 807403522 or 0x30200002 ) and then search for the value in the `result.h` file. You should get the name of the error which can then be searched to find the origination point.

## Decoding Error Codes from a Subprocess or Child Process of Device Update

If the error code you are searching for does NOT come up from the search check to see if it is one that comes from a subcomponent of Device Update which does not record its result codes within Device Update (e.g. Delivery Optimization, APT Child Process, etc.).

Delivery Optmization Error Codes will have a facility code of `13` or `0xD` and extension codes will have reserved common error codes of 0-300 with their associated facility.

These will require manual decoding and use of the faciltiies and codes to check for their origin. Once you have found the facility and/or component you can search for the invokation of the static Extension and Child Proccess Error Code Generator macros in the code base to see where they originate from and/or how to handle them.

## References

See [result.h](../../src/inc/aduc/result.h) for a list of all `extended result codes`.
