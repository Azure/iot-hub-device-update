# Device Update Agent result codes and extended result codes

## Result Code

The followings are Result Code that visible in the Device or Module Twin:

| Result Code | C Macro |
|---|---|
| 0 | ADUC_Result_Failure |
| -1 | ADUC_Result_Failure_Cancelled |
| 500 | ADUC_Result_Download_Success |
| 501 | ADUC_Result_Download_InProgress |
| 502 | ADUC_Result_Download_Skipped_FileExists |
| 503 | ADUC_Result_Download_Skipped_UpdateAlreadyInstalled |
| 504 | ADUC_Result_Download_Skipped_NoMatchingComponents |
| 600 | ADUC_Result_Install_Success |
| 601 | ADUC_Result_Install_InProgress |
| 603 | ADUC_Result_Install_Skipped_UpdateAlreadyInstalled |
| 604 | ADUC_Result_Install_Skipped_NoMatchingComponents |
| 700 | ADUC_Result_Apply_Success |
| 800 | ADUC_Result_Cancel_Success |
| 801 | ADUC_Result_Cancel_UnableToCancel |

See [adu_core.h](../../src/adu/types/adu_core.h) for more detail.

## Extended Result Code Structure (32 bits)
  
```text  
   0 00 00000     Total 4 bytes (32 bits)
   - -- -----
   | |  |
   | |  |
   | |  +---------  Error code (20 bits)
   | |
   | +------------- Component/Area code (8 bits)
   |
   +--------------- Facility code (4 bits) 
 ```

## Decoding an Error Code

Find the facility code ( [**?**] ## ##### ) ?

| [0](#result-codes-from-common-or-unknown-facility-facility-0) | [1](#result-codes-from-lower-platform-layer-facility-1) | [2](#result-codes-from-upper-layer-facility-2) | [3](#update-content-handler-result-codes-facility-3) | [4](#content-downloader-result-codes-facility-4) | 5 | 6 | [7](#components-enumerator-result-codes-facility-7) | [8](#result-codes-from-helper-functions-or-shared-utilities-facility-8) | 9 | A | B | C | [D](#additional-delivery-optimization-downloader-result-codes) | E | F

## Facility Codes

```text
typedef enum tagADUC_Facility
{
    /*indicates errors from unknown sources*/
    ADUC_FACILITY_UNKNOWN = 0x0,

    /*indicates errors from the lower layer*/
    ADUC_FACILITY_LOWERLAYER = 0x1,

    /*indicates errors from the upper layer. */
    ADUC_FACILITY_UPPERLAYER = 0x2,

    /*indicates errors from UPDATE CONTENT HANDLER Extension. */
    ADUC_FACILITY_EXTENSION_UPDATE_CONTENT_HANDLER = 0x3,

    /*indicates errors from CONTENT_DOWNLOADER Extension. */
    ADUC_FACILITY_EXTENSION_CONTENT_DOWNLOADER = 0x4,

    /*indicates errors from COMMUNICATION PROVIDER Extension. */
    ADUC_FACILITY_EXTENSION_COMMUNICATION_PROVIDER = 0x5,

    /*indicates errors from LOG PROVIDER Extension. */
    ADUC_FACILITY_EXTENSION_LOG_PROVIDER = 0x6,

    /*indicates errors from COMPONENT ENUMERATOR Extension. */
    ADUC_FACILITY_EXTENSION_COMPONENT_ENUMERATOR = 0x7,

    /*indicates errors from utility functions. */
    ADUC_FACILITY_UTILITY = 0x8,

    ADUC_FACILITY_UNUSED_9 = 0x9,
    ADUC_FACILITY_UNUSED_A = 0xa,
    ADUC_FACILITY_NSUSED_B = 0xb,
    ADUC_FACILITY_UNUSED_C = 0xc,

    // NOTE: DO retuns HRESULT-like code. To minimize data lost, we assigned it's
    /*indicates errors from Delivery Optimization. */
    ADUC_FACILITY_DELIVERY_OPTIMIZATION = 0xd,

    ADUC_FACILITY_UNUSED_E = 0xe,
    ADUC_FACILITY_UNUSED_F = 0xf,

} ADUC_Facility

```

### Extended Result Codes from Common or Unknown facility (facility #0)

0x0 ## #####

```text
   0 00 00000     Total 4 bytes (32 bits)
   - -- -----
   | |  |
   | |  |
   | |  +---------  Error code (20 bits)
   | |
   | +------------- (Always 00)
   |
   +--------------- Facility code (4 bits) 
 ```

###### POSIX 2001 Standard ErrNos

Note: Errno Values taken from /usr/include/asm-generic/errno-base.h and /usr/include/asm-generic/errno.h

See "man 3 errno" manpage for more details.


| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x00000001 | ADUC_ERC_VADUC_ERC_NOTPERMITTEDALIDATION_FILE_HASH_IS_EMPTY  |
| 0x0000000C | ADUC_ERC_NOMEM (12)  |
| 0x00000083 | ADUC_ERC_NOTRECOVERABLE (131)  |

### Extended Result Codes from the Lower (platform) Layer (facility #1)

0x100 #####

```text
   1 00 00000     Total 4 bytes (32 bits)
   - -- -----
   | |  |
   | |  |
   | |  +---------  Error code (20 bits)
   | |
   | +------------- Component (Update Content Handler) code (8 bits)
   |                0x01 : Cryptographic Component
   |                0x20 - 0xff : Available for customers and partners
   |
   +--------------- Facility code (4 bits) 
 ```

##### Macro for creating extended result codes

```c
static inline ADUC_Result_t MAKE_ADUC_LOWERLAYER_EXTENDEDRESULTCODE(const int value)
{
    return MAKE_ADUC_EXTENDEDRESULTCODE(ADUC_FACILITY_LOWERLAYER, 0 /* non component-specific */, value);
}
```

###### Lower Layer Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x10000001 |ADUC_ERC_LOWERLEVEL_INVALID_UPDATE_ACTION  |
| 0x10000002 |ADUC_ERC_LOWERLEVEL_UPDATE_MANIFEST_VALIDATION_INVALID_HASH  |
| 0x10000003 |ADUC_ERC_LOWERLEVEL_GET_FILE_ENTITY_FAILURE  |
| 0x10000005 |ADUC_ERC_LOWERLEVEL_SANDBOX_CREATE_FAILURE_NO_ADU_USER  |
| 0x10000006 |ADUC_ERC_LOWERLEVEL_SANDBOX_CREATE_FAILURE_NO_ADU_GROUP  |
| 0x10000014 |ADUC_ERC_LOWERLEVEL_WORKFLOW_INIT_ERROR_NULL_PARAM  |

### Extended Result Codes from the Upper Layer (facility #2)

0x100 #####

```text
   2 00 00000     Total 4 bytes (32 bits)
   - -- -----
   | |  |
   | |  |
   | |  +---------  Error code (20 bits)
   | |
   | +------------- (Always 00)
   |
   +--------------- Facility code (4 bits) 
 ```

##### Macro for creating extended result codes

```c
static inline ADUC_Result_t MAKE_ADUC_UPPERLAYER_EXTENDEDRESULTCODE(const int value)
{
    return MAKE_ADUC_EXTENDEDRESULTCODE(ADUC_FACILITY_UPPERLAYER, 0 /* non component-specific */, value);
}
```

###### Upper Layer Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x20000001 |ADUC_ERC_UPPERLEVEL_WORKFLOW_UPDATE_ACTION_UNEXPECTED_STATE  |
| 0x20000002 |ADUC_ERC_UPPERLEVEL_WORKFLOW_INSTALL_ACTION_IN_UNEXPECTED_STATE  |

### Update Content Handler Result Codes (facility #3)

```text
   3 00 00000     Total 4 bytes (32 bits)
   - -- -----
   | |  |
   | |  |
   | |  +---------  Error code (20 bits)
   | |
   | +------------- Component (Update Content Handler) code (8 bits)
   |                0x00 - 0x1f : RESERVED for DU-Provided content handlers
   |                0x20 - 0xff : Available for customers and partners
   |
   +--------------- Facility code (4 bits) 
 ```

#### Update Content Handler Lookup

3 [**??**] ######

[00]() | [01](#swupdate-content-handler-result-codes-0x301) | [02](#apt-update-content-handler-result-codes-0x302) | [04](#steps-handler-result-codes-0x304) | [05](#script-handler-result-codes-0x305) | [20]()

```text
typedef enum tagADUC_Content_Handler
{
    /*indicates a common error from any handler. */
    ADUC_CONTENT_HANDLER_COMMON = 0x00,

    /*indicates errors from SWUPDATE Update Handler. */
    ADUC_CONTENT_HANDLER_SWUPDATE = 0x01,

    /*indicates errors from APT Update Handler. */
    ADUC_CONTENT_HANDLER_APT = 0x02,

    /*indicates errors from Simulator Update Handler. */
    ADUC_CONTENT_HANDLER_SIMULATOR = 0x03,

    /*indicates errors from Steps Handler. */
    ADUC_CONTENT_HANDLER_STEPS = 0x04,

    /*indicates errors from Script Update Handler. */
    ADUC_CONTENT_HANDLER_SCRIPT = 0x05,

    /*indicates errors from Custom Update handlers. */
    ADUC_CONTENT_HANDLER_EXTERNAL = 0x20,
} ADUC_Content_Handler;
```

#### SWUpdate Content Handler Result Codes (0x301#####)

##### Macro for creating extended result codes

```c
static inline ADUC_Result_t MAKE_ADUC_SWUPDATE_HANDLER_EXTENDEDRESULTCODE(const int32_t value)
{
    return MAKE_ADUC_CONTENT_HANDLER_EXTENDEDRESULTCODE(ADUC_CONTENT_HANDLER_SWUPDATE, value);
}
```

###### General Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x30100001 |ADUC_ERC_SWUPDATE_HANDLER_EMPTY_VERSION  |
| 0x30100002 | ADUC_ERC_SWUPDATE_HANDLER_PREPARE_FAILURE_UNKNOWN_UPDATE_VERSION |
| 0x30100003 | ADUC_ERC_SWUPDATE_HANDLER_PREPARE_FAILURE_WRONG_UPDATE_VERSION |
| 0x30100004 |ADUC_ERC_SWUPDATE_HANDLER_PREPARE_FAILURE_WRONG_FILECOUNT |
| 0x30100005 |ADUC_ERC_SWUPDATE_HANDLER_PREPARE_FAILURE_BAD_FILE_ENTITY |

###### Download Related Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x30100101 |ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_UNKNOW_UPDATE_VERSION |
| 0x30100102 |ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_WRONG_UPDATE_VERSION |
| 0x30100103 |ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_WRONG_FILECOUNT
| 0x30100104 |ADUC_ERC_SWUPDATE_HANDLER_DOWNLOADE_BAD_FILE_ENTITY |

###### Install Related Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x30100201 |ADUC_ERC_SWUPDATE_HANDLER_INSTALL_FAILURE_CANNOT_OPEN_WORKFOLDER |
| 0x30100102 |ADUC_ERC_SWUPDATE_HANDLER_INSTALL_FAILURE_BAD_FILE_ENTITY

###### Apply Related Result Codes

```text
    None
```

###### Cancel Related Result Codes

```text
    None
```

###### IsInstalled Related Result Codes

```text
    None
```

###### Extended Result Codes from Child Processes (0x30101000 + exitCode)

All result codes start from 0x30101000 are exit code(s) returned from a child process that invoked by SWUpdate Content Handler.

#### APT Update Content Handler Result Codes (0x302#####)

##### Macro for creating extended result codes

```c
static inline ADUC_Result_t MAKE_ADUC_APT_HANDLER_EXTENDEDRESULTCODE(const int32_t value)
{
    return MAKE_ADUC_CONTENT_HANDLER_EXTENDEDRESULTCODE(ADUC_CONTENT_HANDLER_APT, value);
}
```

###### General Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x30200000 |ADUC_ERC_APT_HANDLER_ERROR_NONE  |
| 0x30200001 |ADUC_ERC_APT_HANDLER_INITIALIZATION_FAILURE  |
| 0x30200002 |ADUC_ERC_APT_HANDLER_INVALID_PACKAGE_DATA  |
| 0x30200003 |ADUC_ERC_APT_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_VERSION  |
| 0x30200004 |ADUC_ERC_APT_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_FILECOUNT  |
| 0x30200005 |ADUC_ERC_APT_HANDLER_GET_FILEENTITY_FAILURE  |
| 0x30200006 |ADUC_ERC_APT_HANDLER_INSTALLCRITERIA_PERSIST_FAILURE  |

###### Download Related Extended Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x30200100 | ADUC_ERC_APT_HANDLER_PACKAGE_DOWNLOAD_FAILURE |

###### Install Related Extended Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x30200200 |ADUC_ERC_APT_HANDLER_PACKAGE_INSTALL_FAILURE|

###### Apply Related Result Codes

```text
    None
```

###### Cancel Related Extended Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x30200400 |ADUC_ERC_APT_HANDLER_PACKAGE_CANCEL_FAILURE|

###### IsInstalled Related Extended Result Codes

```text
    None
```

#### Steps Handler Result Codes (0x304#####)

##### Macro for creating extended result codes

```c
static inline ADUC_Result_t MAKE_ADUC_STEPS_HANDLER_EXTENDEDRESULTCODE(const int32_t value)
{
    return MAKE_ADUC_CONTENT_HANDLER_EXTENDEDRESULTCODE(ADUC_CONTENT_HANDLER_STEPS, value);
}
```

###### General Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x30400001 |ADUC_ERC_STEPS_HANDLER_GET_FILE_ENTITY_FAILURE  |
| 0x30400002 |ADUC_ERC_STEPS_HANDLER_INVALID_CHILD_MANIFEST_FILENAME  |
| 0x30400003 |ADUC_ERC_STEPS_HANDLER_CHILD_WORKFLOW_CREATE_FAILED  |
| 0x30400004 |ADUC_ERC_STEPS_HANDLER_CHILD_WORKFLOW_INSERT_FAILED  |
| 0x30400005 |ADUC_ERC_STEPS_HANDLER_GET_REF_STEP_COMPATIBILITY_FAILED  |
| 0x30400006 |ADUC_ERC_STEPS_HANDLER_SET_SELECTED_COMPONENTS_FAILED  |
| 0x30400007 |ADUC_ERC_STEPS_HANDLER_GET_STEP_COMPATIBILITY_FAILED  |
| 0x30400008 |ADUC_ERC_STEPS_HANDLER_SET_SELECTED_COMPONENTS_FAILURE  |
| 0x30400009 |ADUC_ERC_STEPS_HANDLER_NESTED_REFERENCE_STEP_DETECTED  |
| 0x3040000A |ADUC_ERC_STEPS_HANDLER_INVALID_COMPONENTS_DATA  |
| 0x3040000B |ADUC_ERC_STEPS_HANDLER_CREATE_SANDBOX_FAILURE  |


###### Download Related Extended Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x30400100 | ADUC_ERC_STEPS_HANDLER_DOWNLOAD_FAILURE_UNKNOWNEXCEPTION |
| 0x30400101 | ADUC_ERC_STEPS_HANDLER_DOWNLOAD_FAILURE_MISSING_CHILD_WORKFLOW |
| 0x30400102 | ADUC_ERC_STEPS_HANDLER_DOWNLOAD_UNKNOWN_EXCEPTION_DOWNLOAD_CONTENT |

###### Install Related Extended Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x30400200 |ADUC_ERC_STEPS_HANDLER_INSTALL_FAILURES_UNKNOWNEXCEPTION|
| 0x30400201 |ADUC_ERC_STEPS_HANDLER_INSTALL_FAILURE_MISSING_CHILD_WORKFLOW|
| 0x30400202 |ADUC_ERC_STEPS_HANDLER_INSTALL_UNKNOWN_EXCEPTION_INSTALL_CHILD_STEP|
| 0x30400203 |ADUC_ERC_STEPS_HANDLER_INSTALL_UNKNOWN_EXCEPTION_APPLY_CHILD_STEP|

###### Apply Related Result Codes

```text
    None
```

###### IsInstalled Related Extended Result Codes

```text
    None
```


#### Script Handler Result Codes (0x305#####)

##### Macro for creating extended result codes

```c
static inline ADUC_Result_t MAKE_ADUC_STEPS_HANDLER_EXTENDEDRESULTCODE(const int32_t value)
{
    return MAKE_ADUC_CONTENT_HANDLER_EXTENDEDRESULTCODE(ADUC_CONTENT_HANDLER_SCRIPT, value);
}
```

###### General Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x30500000 |ADUC_ERC_SCRIPT_HANDLER_ERROR_NONE |
| 0x30500001 |ADUC_ERC_SCRIPT_HANDLER_SET_SELECTED_COMPONENTS_FAILURE |
| 0x30500002 |ADUC_ERC_SCRIPT_HANDLER_MISSING_INSTALLED_CRITERIA |
| 0x30500003 |ADUC_ERC_SCRIPT_HANDLER_TOO_MANY_COMPONENTS |
| 0x30500004 |ADUC_ERC_SCRIPT_HANDLER_MISSING_PRIMARY_SCRIPT_FILE |
| 0x30500005 |ADUC_ERC_SCRIPT_HANDLER_MISSING_SCRIPTFILENAME_PROPERTY |
| 0x30500006 |ADUC_ERC_SCRIPT_HANDLER_CREATE_SANDBOX_FAILURE |
| 0x30500007 |ADUC_ERC_SCRIPT_HANDLER_INVALID_COMPONENTS_DATA |

###### Download Related Extended Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x30500101 | ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_ERROR_NULL_WORKFLOW |
| 0x30500102 | ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_FAILURE_INVALID_FILE_COUNT |
| 0x30500103 | ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_FAILURE_GET_PRIMARY_FILE_ENTITY |
| 0x30500104 | ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_FAILURE_GET_PAYLOAD_FILE_ENTITY |
| 0x305001FE | ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_PAYLOAD_FILE_FAILURE_UNKNOWNEXCEPTION |
| 0x305001FF | ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_PRIMARY_FILE_FAILURE_UNKNOWNEXCEPTION |

###### Install Related Extended Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x30500201 |ADUC_ERC_SCRIPT_HANDLER_INSTALL_ERROR_NULL_WORKFLOW|
| 0x30500202 |ADUC_ERC_SCRIPT_HANDLER_INSTALL_INSTALLITEM_BAD_DATA|
| 0x30500203 |ADUC_ERC_SCRIPT_HANDLER_INSTALL_INSTALLITEM_NO_UPDATE_TYPE|
| 0x30500204 |ADUC_ERC_SCRIPT_HANDLER_INSTALL_FAILURE_MISSING_PRIMARY_SCRIPT_FILE|
| 0x30500205 |ADUC_ERC_SCRIPT_HANDLER_INSTALL_FAILURE_PARSE_RESULT_FILE|
| 0x305002FF |ADUC_ERC_SCRIPT_HANDLER_INSTALL_FAILURE_UNKNOWNEXCEPTION|

###### Apply Related Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x3050103FF |ADUC_ERC_SCRIPT_HANDLER_APPLY_FAILURE_UNKNOWNEXCEPTION| 

###### IsInstalled Related Extended Result Codes

```text
    None
```

###### Extended Result Codes (from child process)

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x30501### |ADUC_ERC_SCRIPT_HANDLER_CHILD_PROCESS_FAILURE_EXITCODE | The last 12 bits contains exit code from child process |

### Content Downloader Result Codes (facility #4)

```text
   4 00 00000     Total 4 bytes (32 bits)
   - -- -----
   | |  |
   | |  |
   | |  +---------  Error code (20 bits)
   | |
   | +------------- Component (Content Downloader) code (8 bits)
   |
   +--------------- Facility code (4 bits) 
 ```

#### Content Downloader Lookup

4 [**??**] ######

[00](#content-downloader-common-result-codes) | [01](#delivery-optimization-downloader-result-codes) | [03](#curl-downloader-result-codes)

```text
typedef enum tagADUC_Content_Downloader
{
    /*indicates common errors */
    ADUC_CONTENT_DOWNLOADER_COMMON = 0x00,

    /*indicates errors from Delivery Optimzation agent. */
    ADUC_CONTENT_DOWNLOADER_DELIVERY_OPTIMIZATION = 0x01,

    /*indicates errors from Curl Downloader. */
    ADUC_CONTENT_DOWNLOADER_CURL_DOWNLOADER = 0x03,

} ADUC_Content_Downloader
```

###### Content Downloader Common Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x40000001 |ADUC_ERC_CONTENT_DOWNLOADER_CREATE_FAILURE_NO_SYMBOL  |
| 0x40000002 |ADUC_ERC_CONTENT_DOWNLOADER_INITIALIZEPROC_NOTIMP  |
| 0x40000003 |ADUC_ERC_CONTENT_DOWNLOADER_DOWNLOADPROC_NOTIMP  |
| 0x40000004 |ADUC_ERC_CONTENT_DOWNLOADER_INITIALIZE_EXCEPTION  |
| 0x40000005 |ADUC_ERC_CONTENT_DOWNLOADER_DOWNLOAD_EXCEPTION  |
| 0x40000006 |ADUC_ERC_CONTENT_DOWNLOADER_INVALID_FILE_ENTITY  |
| 0x40000007 |ADUC_ERC_CONTENT_DOWNLOADER_INVALID_DOWNLOAD_URI  |
| 0x40000008 |ADUC_ERC_CONTENT_DOWNLOADER_FILE_HASH_TYPE_NOT_SUPPORTED  |
| 0x40000009 |ADUC_ERC_CONTENT_DOWNLOADER_BAD_CHILD_MANIFEST_FILE_PATH  |
| 0x4000000a |ADUC_ERC_CONTENT_DOWNLOADER_CANNOT_DELETE_EXISTING_FILE  |
| 0x4000000b |ADUC_ERC_CONTENT_DOWNLOADER_INVALID_FILE_ENTITY_NO_HASHES |

###### Delivery Optimization Downloader Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x40100001 |ADUC_ERROR_DELIVERY_OPTIMIZATION_DOWNLOADER_NOT_INITIALIZE  |
| 0x40100002 |ADUC_ERROR_DELIVERY_OPTIMIZATION_DOWNLOADER_BAD_INIT_DATA  |
| 0x40101000 + (exitCode) |ADUC_ERROR_DELIVERY_OPTIMIZATION_DOWNLOADER_EXTERNAL_FAILURE  |

###### Additional Delivery Optimization Downloader Result Codes

Another set of extended result codes returned from the Delivery Optimization start 0xD0000000. To understand these result codes, replace 0xD with 0x8, then look up a matching WIN32 HRESULT.

###### Curl Downloader Result Codes

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x40300001 |ADUC_ERROR_CURL_DOWNLOADER_INVALID_FILE_HASH  |
| 0x40301000 + (exitCode) |ADUC_ERROR_CURL_DOWNLOADER_EXTERNAL_FAILURE  |

### Component Enumerator Result Codes (facility #7)

7 00 #####

##### Macro for creating extended result codes

```c
static inline ADUC_Result_t MAKE_ADUC_COMPONENT_ENUMERATOR_EXTENDEDRESULTCODE(const int value)
{
    return MAKE_ADUC_EXTENDEDRESULTCODE(ADUC_FACILITY_EXTENSION_COMPONENT_ENUMERATOR, 0, value);
}
```

###### Component Enumerator Reserved Result Codes (0x7000000 - 0x70000200)

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x70000001 |ADUC_ERC_COMPONENT_ENUMERATOR_GETALLCOMPONENTS_NOTIMP  |
| 0x70000002 |ADUC_ERC_COMPONENT_ENUMERATOR_SELECTCOMPONENTS_NOTIMP  |
| 0x70000003 |ADUC_ERC_COMPONENT_ENUMERATOR_FREECOMPONENTSDATASTRING_NOTIMP  |
| 0x70000004 |ADUC_ERC_COMPONENT_ENUMERATOR_EXCEPTION_GETALLCOMPONENTS  |
| 0x70000005 |ADUC_ERC_COMPONENT_ENUMERATOR_EXCEPTION_SELECTCOMPONENTS  |
| 0x70000006 |ADUC_ERC_COMPONENT_ENUMERATOR_EXCEPTION_FREECOMPONENTSDATASTRING  |

**Note:** Custom component enumerator extension should return error codes between 0x70000200 and 0x700FFFFF

### Extended Result Codes from helper functions or shared utilities (facility #8)

0x8 [??] #####

| [01](#crypto-related-result-codes-0x801---0x801fffff) | [03](#data-parser-result-codes-0x803---0x803fffff) | [04](#workflow-utility-result-codes-0x804---0x804fffff)

##### Macro for creating extended result codes

```c
/**
 * @brief Macros to convert results from helper functions or shared utilities.
 */
static inline ADUC_Result_t MAKE_ADUC_UTILITIES_EXTENDEDRESULTCODE(const int component, const int value)
{
    return MAKE_ADUC_EXTENDEDRESULTCODE(ADUC_FACILITY_UTILITY, component, value);
}

```

##### Components

```c
typedef enum tagADUC_Component
{
    /*indicates errors from Crypto-related functions or utilities. */
    ADUC_COMPONENT_CRYPTO = 0x01,

    /*indicates errors from Component Handler Factory. */
    ADUC_COMPONENT_COMPONENT_HANDLER_FACTORY = 0x02,

    /*indicates errors from parsers. */
    ADUC_COMPONENT_UPDATE_DATA_PARSER = 0x03,

    /*indicates errors from Workflow Utilities or Function. */
    ADUC_COMPONENT_WORKFLOW_UTIL = 0x04,
} ADUC_Component;

```

###### Crypto Related Result Codes (0x801##### - 0x801FFFFF)

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x80100001 |ADUC_ERC_VALIDATION_FILE_HASH_IS_EMPTY  |
| 0x80100002 |ADUC_ERC_VALIDATION_FILE_HASH_TYPE_NOT_SUPPORTED  |
| 0x80100003 |ADUC_ERC_VALIDATION_FILE_HASH_INVALID_HASH  |

###### Data Parser Result Codes (0x803##### - 0x803FFFFF)

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x80300000 |ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_INVALID_ACTION_JSON  |
| 0x80300001 |ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_MANIFEST_VALIDATION_FAILED  |
| 0x80300002 |ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_NO_UPDATE_ACTION  |
| 0x80300003 |ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_NO_UPDATE_ID  |
| 0x80300004 |ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_NO_UPDATE_TYPE  |
| 0x80300005 |ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_BAD_UPDATE_TYPE  |
| 0x80300006 |ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_NO_INSTALLEDCRITERIA  |
| 0x80300007 |ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_BAD_FILES_OR_FILEURLS  |
| 0x80300008 |ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_NO_UPDATE_MANIFEST  |
| 0x80300009 |ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_BAD_UPDATE_MANIFEST  |
| 0x8030000A |ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_UNSUPPORTED_UPDATE_MANIFEST_VERSION  |
| 0x8030000B |ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_MISSING_DETACHED_UPDATE_MANIFEST_ENTITY |
| 0x8030000C |ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_BAD_DETACHED_UPDATE_MANIFEST |
| 0x8030000D |ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_DETACHED_UPDATE_MANIFEST_DOWNLOAD_FAILED |

###### Workflow Utility Result Codes (0x804##### - 0x804FFFFF)

| Extended Result Code | C Macro | Note |
|:----|:----|:----|
| 0x80400001 |ADUC_ERC_UTILITIES_WORKFLOW_UTIL_ERROR_BAD_PARAM  |
| 0x80400002 |ADUC_ERC_UTILITIES_WORKFLOW_UTIL_ERROR_NO_MEM  |
| 0x80400003 |ADUC_ERC_UTILITIES_WORKFLOW_UTIL_INVALID_ACTION_JSON_STRING  |
| 0x80400004 |ADUC_ERC_UTILITIES_WORKFLOW_UTIL_INVALID_ACTION_JSON_FILE  |
| 0x80400005 |ADUC_ERC_UTILITIES_WORKFLOW_UTIL_INVALID_UPDATE_ID  |
| 0x80400006 |ADUC_ERC_UTILITIES_WORKFLOW_UTIL_COPY_UPDATE_ACTION_FROM_BASE_FAILURE  |
| 0x80400007 |ADUC_ERC_UTILITIES_WORKFLOW_UTIL_COPY_UPDATE_MANIFEST_FROM_BASE_FAILURE  |
| 0x80400008 |ADUC_ERC_UTILITIES_WORKFLOW_UTIL_PARSE_INSTRUCTION_ENTRY_FAILURE  |
| 0x80400009 |ADUC_ERC_UTILITIES_WORKFLOW_UTIL_PARSE_INSTRUCTION_ENTRY_NO_UPDATE_TYPE  |
| 0x8040000A |ADUC_ERC_UTILITIES_WORKFLOW_UTIL_COPY_UPDATE_ACTION_SET_UPDATE_TYPE_FAILURE  |

## References

See [result.h](../../src/inc/aduc/result.h) for a list of all `extended result codes`.
