# Update Content Handler Extension

To support various software installation technologies and tools, the 2021 Q2 release of the Device Update Agent will support the Update Content Handler extension.  
  
The older versions of Device Update Agent only support two Update Types, `microsoft/apt:1`, and `microsoft/swupdate:1`, and only accepting one Update Type (selected at build time) per each flavor of the agent.
  
E.g., deviceupdate-agent Debian Package only supports `microsoft/apt:1` Update Type. The Device Update agent built as part of the reference Yocto Raspberry Pi 3 image only supports `microsoft/swupdate:1`.

## Quick Jump

- [Update Content Handler Extension](#update-content-handler-extension)
  - [Quick Jump](#quick-jump)
    - [How To Implement An Update Content Handler](#how-to-implement-an-update-content-handler)
    - [Installing An Update Content Handler](#installing-an-update-content-handler)
    - [Registering The Update Content Handler with Device Update Agent](#registering-the-update-content-handler-with-device-update-agent)
      - [Handler Registration File Format](#handler-registration-file-format)
  - [List Of Supported Update Content Handler](#list-of-supported-update-content-handler)

### How To Implement An Update Content Handler

The Update Content Handler is a linux shared library that support following APIs:

```c
/**
 * @brief Defines the type of an ADUC_Result.
 */
typedef int32_t ADUC_Result_t;

/**
 * @brief Defines an ADUC_Result object which is used to indicate status.
 */
typedef struct tagADUC_Result
{
    ADUC_Result_t ResultCode; /**< Method-specific result. Value > 0 indicates success. */
    ADUC_Result_t ExtendedResultCode; /**< Implementation-specific extended result code. */
    char* ResultDetails; /**< Implementation-specific result details string. */
} ADUC_Result;

/**
 * @brief Encapsulates the hash and the hash type
 */
typedef struct tagADUC_Hash
{
    char* value; /** The value of the actual hash */
    char* type; /** The type of hash held in the entry*/
} ADUC_Hash;

/**
 * @brief Describes a specific file to download.
 */
typedef struct tagADUC_FileEntity
{
    char* TargetFilename; /**< File name to store content in DownloadUri to. */
    char* DownloadUri; /**< The URI of the file to download. */
    ADUC_Hash* Hash; /**< Array of ADUC_Hashes containing the hash options for the file*/
    size_t HashCount; /**< Total number of hashes in the array of hashes */
    char* FileId; /**< Id for the file */
} ADUC_FileEntity;

/**
 * @brief Describes Prepare info
 */
typedef struct tagADUC_PrepareInfo
{
    char* updateType; /**< Handler UpdateType. */
    char* updateTypeName; /**< Provider/Name in the UpdateType string. */
    unsigned int updateTypeVersion; /**< Version number in the UpdateType string. */
    unsigned int fileCount; /**< Number of files in #Files list. */
    ADUC_FileEntity* files; /**< Array of #ADUC_FileEntity objects describing what to download. */
    char* targetComponentContext; /**< A serialized JSON string contains information about the target component.>
} ADUC_PrepareInfo;

/**
 * @interface ContentHandler
 * @brief Interface for content specific handler implementations.
 */
class ContentHandler
{
public:
    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    ContentHandler(const ContentHandler&) = delete;
    ContentHandler& operator=(const ContentHandler&) = delete;
    ContentHandler(ContentHandler&&) = delete;
    ContentHandler& operator=(ContentHandler&&) = delete;

    virtual ~ContentHandler() = default;

    virtual ADUC_Result Prepare(const ADUC_PrepareInfo* prepareInfo) = 0;
    virtual ADUC_Result Download() = 0;
    virtual ADUC_Result Install() = 0;
    virtual ADUC_Result Apply() = 0;
    virtual ADUC_Result Cancel() = 0;
    virtual ADUC_Result IsInstalled(const std::string& installedCriteria) = 0;

protected:
    ContentHandler() = default;
};

/**
 * @brief Constructor
 * This function instantiates and returns ContentHandler object.
 */
std::unique_ptr<ContentHandler> CreateUpdateContentHandlerExtension(const ContentHandlerCreateData& data)

```

See [Microsoft APT Update Content Handler](../../../src/content_handlers/apt_handler) for example.

### Installing An Update Content Handler

All Update Content Handlers should be install at:

```
/usr/lib/adu/handlers.d/<first part of Update Type>/<second part of Update Type>/<update type version>/` directory.  
```

For example, `libmicrosoft-apt-1.so` is a handler for the `microsoft/apt:1` Update Type.  
This handler should be installed at:  

```
/usr/lib/adu/handlers.d/microsoft/apt/1/libmicrosoft-apt-1.so
```

### Registering The Update Content Handler with Device Update Agent

To register the handler, create a registration file, and place it at following directory.

```sh
/etc/adu/content-handlers.d/
```

#### Handler Registration File Format

```json
{
    "updateType":"<update-type>",
    "path":"<full-path-to-shared-library>",
    "sha256":"<file-hash>"
}
```

**Example registration file:**

```json
{
    "updateType":"microsoft/apt:1",
    "path":"/user/lib/adu/handlers.d/microsoft/apt/1/libmicrosoft-apt-1.so",
    "sha256":"xAbsdf802x3233="
}
```

Or, the same information can be specified in [Device Update Agent Configuration File](./configuration-manager.md#configuration-file-format) in `contentHandlers` array.  
  
For example:

```json
"contentHandlers":[
    {
        "updateType":"microsoft/apt:1",
        "path":"/user/lib/adu/handlers.d/microsoft/apt/1/libmicrosoft-apt-1.so",
        "sha256":"xAbsdf802x3233="
    },
```

> Note that the specified `sha256` hash will be used to validate the handler file integrity.

## List Of Supported Update Content Handler

- Microsoft APT Update Content Handler
- Microsoft SWUpdate Content Handler
- [Microsoft Multi Component Update Content Handler](../multi-component-update/overview.md)