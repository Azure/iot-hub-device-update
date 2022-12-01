/**
 * @file extension_utils.c
 * @brief Implements utilities for working with Device Update extension.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/extension_utils.h"
#include "aduc/hash_utils.h"
#include "aduc/logging.h"
#include "aduc/parser_utils.h"
#include "aduc/path_utils.h" // SanitizePathSegment
#include "aduc/string_c_utils.h"
#include "aduc/system_utils.h"

#include <ctype.h> // isalnum
#include <grp.h> // for getgrnam
#include <parson.h> // for JSON_*, json_*
#include <pwd.h> // for getpwnam
#include <stdio.h> // for FILE
#include <stdlib.h> // for calloc
#include <strings.h> // for strcasecmp
#include <sys/stat.h> // for stat, struct stat

#include <azure_c_shared_utility/azure_base64.h>
#include <azure_c_shared_utility/buffer_.h>
#include <azure_c_shared_utility/crt_abstractions.h> // for mallocAndStrcpy_s
#include <azure_c_shared_utility/sha.h> // for SHAversion

/**
 * @brief Get the Extension File Entity object
 *
 * @param[in] extensionRegFile A full path to the extension registration file.
 * @param[in,out] fileEntity An output buffer to hold file entity data.
 * @return bool Returns 'true' if succeeded.
 */
bool GetExtensionFileEntity(const char* extensionRegFile, ADUC_FileEntity* fileEntity)
{
    bool found = false;
    size_t tempHashCount = 0;
    ADUC_Hash* tempHash = NULL;
    const char* fileName = NULL;

    JSON_Value* rootValue = json_parse_file(extensionRegFile);
    if (rootValue == NULL)
    {
        Log_Info("Cannot open an extension registration file. ('%s')", extensionRegFile);
        goto done;
    }

    JSON_Object* fileObj = json_value_get_object(rootValue);

    const JSON_Object* hashObj = json_object_get_object(fileObj, ADUCITF_FIELDNAME_HASHES);

    if (hashObj == NULL)
    {
        Log_Error("No hash for file '%s'.", extensionRegFile);
        goto done;
    }

    tempHash = ADUC_HashArray_AllocAndInit(hashObj, &tempHashCount);
    if (tempHash == NULL)
    {
        Log_Error("Unable to parse hashes for file '%s'", extensionRegFile);
        goto done;
    }

    fileName = json_object_get_string(fileObj, ADUCITF_FIELDNAME_FILENAME);

    if (mallocAndStrcpy_s(&(fileEntity->TargetFilename), fileName) != 0)
    {
        goto done;
    }

    fileEntity->Hash = tempHash;
    fileEntity->HashCount = tempHashCount;

    found = true;

done:
    if (!found)
    {
        free(fileEntity->TargetFilename);
        free(fileEntity->FileId);
        if (tempHash != NULL)
        {
            ADUC_Hash_FreeArray(tempHashCount, tempHash);
        }
    }

    json_value_free(rootValue);

    return found;
}

/**
 * @brief Find a handler extension file entity for the specified @p handlerId.
 *
 * @param handlerId A string represents Update Type or Step Handler Type.
 * @param fileEntity An output file entity.
 * @return True if a handler for specified @p handlerId is available.
 */
static bool GetHandlerExtensionFileEntity(
    const char* handlerId, const char* extensionDir, const char* regFileName, ADUC_FileEntity* fileEntity)
{
    bool found = false;

    if (IsNullOrEmpty(handlerId))
    {
        Log_Error("Invalid handler identifier.");
        return false;
    }

    if (fileEntity == NULL)
    {
        Log_Error("Invalid output buffer.");
        return false;
    }

    memset(fileEntity, 0, sizeof(*fileEntity));

    STRING_HANDLE folderName = SanitizePathSegment(handlerId);

    STRING_HANDLE path = STRING_construct_sprintf("%s/%s/%s", extensionDir, STRING_c_str(folderName), regFileName);

    found = GetExtensionFileEntity(STRING_c_str(path), fileEntity);

    STRING_delete(folderName);
    STRING_delete(path);

    return found;
}

/**
 * @brief Find a content handler for the specified UpdateType @p updateType.
 *
 * @param updateType Update type string.
 * @param fileEntity An output file entity.
 * @return True if an update content handler for the specified @p updateType is available.
 */
bool GetUpdateContentHandlerFileEntity(const char* updateType, ADUC_FileEntity* fileEntity)
{
    return GetHandlerExtensionFileEntity(
        updateType, ADUC_UPDATE_CONTENT_HANDLER_EXTENSION_DIR, ADUC_UPDATE_CONTENT_HANDLER_REG_FILENAME, fileEntity);
}

/**
 * @brief Find a download handler for the specified DownloadHandler @p downloadHandlerId.
 *
 * @param downloadHandlerId The download handler id string.
 * @param fileEntity An output file entity.
 * @return True if an update content handler for the specified @p downloadHandlerId is available.
 */
bool GetDownloadHandlerFileEntity(const char* downloadHandlerId, ADUC_FileEntity* fileEntity)
{
    return GetHandlerExtensionFileEntity(
        downloadHandlerId, ADUC_DOWNLOAD_HANDLER_EXTENSION_DIR, ADUC_DOWNLOAD_HANDLER_REG_FILENAME, fileEntity);
}

/**
 * @brief Register a handler for specified handler id.
 *
 * @param handlerId A string represents handler id. This can be an Update-Type, or Step-Handler-Type.
 * @param handlerFilePath A full path to the handler shared-library file.
 * @param handlerExtensionDir An extension directory.
 * @param handlerRegistrationFileName A handler registration file name.
 * @return Returns true if the handler successfully registered.
 */
static bool RegisterHandlerExtension(
    const char* handlerId,
    const char* handlerFilePath,
    const char* handlerExtensionDir,
    const char* handlerRegistrationFileName)
{
    bool success = false;
    STRING_HANDLE folderName = NULL;
    STRING_HANDLE dir = NULL;
    STRING_HANDLE content = NULL;
    STRING_HANDLE outFilePath = NULL;
    FILE* outFile = NULL;
    char* hash = NULL;

    Log_Debug("Registering handler for '%s', file: %s", handlerId, handlerFilePath);

    if (IsNullOrEmpty(handlerId))
    {
        Log_Error("Invalid handler identifier.");
        goto done;
    }

    if (IsNullOrEmpty(handlerFilePath))
    {
        Log_Error("Invalid handler extension file path.");
        goto done;
    }

    folderName = SanitizePathSegment(handlerId);
    if (folderName == NULL)
    {
        Log_Error("Cannot generate a folder name from an Update Type.");
        goto done;
    }

    dir = STRING_construct_sprintf("%s/%s", handlerExtensionDir, STRING_c_str(folderName));
    if (dir == NULL)
    {
        goto done;
    }

    // Note: the return value may point to a static area,
    // and may be overwritten by subsequent calls to getpwent(3), getpwnam(), or getpwuid().
    // (Do not pass the returned pointer to free(3).)
    struct passwd* pwd = getpwnam(ADUC_FILE_USER);
    if (pwd == NULL)
    {
        Log_Error("Cannot verify credential of 'adu' user.");
        goto done;
    }
    uid_t aduUserId = pwd->pw_uid;
    pwd = NULL;

    // Note: The return value may point to a static area,
    // and may be overwritten by subsequent calls to getgrent(3), getgrgid(), or getgrnam().
    // (Do not pass the returned pointer to free(3).)
    struct group* grp = getgrnam(ADUC_FILE_GROUP);
    if (grp == NULL)
    {
        Log_Error("Cannot get 'adu' group info.");
        goto done;
    }
    gid_t aduGroupId = grp->gr_gid;
    grp = NULL;

    Log_Debug("Creating the extension folder ('%s'), uid:%d, gid:%d", STRING_c_str(dir), aduUserId, aduGroupId);
    int dir_result = ADUC_SystemUtils_MkDirRecursive(STRING_c_str(dir), aduUserId, aduGroupId, S_IRWXU | S_IRWXG);
    if (dir_result != 0)
    {
        Log_Error("Cannot create a folder for registration file. ('%s')", STRING_c_str(dir));
        goto done;
    }

    struct stat bS;
    if (stat(handlerFilePath, &bS) != 0)
    {
        goto done;
    }

    long fileSize = bS.st_size;
    if (!ADUC_HashUtils_GetFileHash(handlerFilePath, SHA256, &hash))
    {
        goto done;
    }

    content = STRING_construct_sprintf(
        "{\n"
        "   \"fileName\":\"%s\",\n"
        "   \"sizeInBytes\":%d,\n"
        "   \"hashes\": {\n"
        "        \"sha256\":\"%s\"\n"
        "   },\n"
        "   \"handlerId\":\"%s\"\n"
        "}\n",
        handlerFilePath,
        fileSize,
        hash,
        handlerId);

    if (content == NULL)
    {
        Log_Error("Cannot compose the handler registration information.");
        goto done;
    }

    outFilePath = STRING_construct_sprintf("%s/%s", STRING_c_str(dir), handlerRegistrationFileName);
    outFile = fopen(STRING_c_str(outFilePath), "w");
    if (outFile == NULL)
    {
        Log_Error("Cannot open file: %s", STRING_c_str(outFilePath));
        goto done;
    }

    int ref = fputs(STRING_c_str(content), outFile);
    if (ref < 0)
    {
        Log_Error(
            "Failed to write the handler registration information to file. File:%s, Content:%s",
            STRING_c_str(dir),
            STRING_c_str(content));
        goto done;
    }

    // Print directly to stdout. Since this will be seen by customer,
    // we don't want to show any 'log' info (e.g., time stamp, log level.)
    printf(
        "Successfully registered a handler for '%s'. Registration file: %s.\n", handlerId, STRING_c_str(outFilePath));
    success = true;

done:
    if (outFile != NULL)
    {
        fclose(outFile);
    }

    STRING_delete(outFilePath);
    STRING_delete(dir);
    free(hash);

    return success;
}

/**
 * @brief Register a Handler for the specified UpdateType @p updateType.
 *
 * @param updateType Update type string.
 * @param handlerFilePath A full path to the handler shared-library file.
 * @return Returns true if the handler successfully registered.
 */
bool RegisterUpdateContentHandler(const char* updateType, const char* handlerFilePath)
{
    return RegisterHandlerExtension(
        updateType,
        handlerFilePath,
        ADUC_UPDATE_CONTENT_HANDLER_EXTENSION_DIR,
        ADUC_UPDATE_CONTENT_HANDLER_REG_FILENAME);
}

/**
 * @brief Register a Handler for the specified download handler id.
 *
 * @param downloadHandlerId The download handler id string.
 * @param handlerFilePath A full path to the handler shared-library file.
 * @return Returns true if the handler successfully registered.
 */
bool RegisterDownloadHandler(const char* downloadHandlerId, const char* handlerFilePath)
{
    return RegisterHandlerExtension(
        downloadHandlerId, handlerFilePath, ADUC_DOWNLOAD_HANDLER_EXTENSION_DIR, ADUC_DOWNLOAD_HANDLER_REG_FILENAME);
}

/**
 * @brief Register a component enumerator extension.
 * @param extensionFilePath A full path to an extension to register.
 * @return Returns true if the extension successfully registered.
 */
bool RegisterComponentEnumeratorExtension(const char* extensionFilePath)
{
    return RegisterExtension(ADUC_COMPONENT_ENUMERATOR_EXTENSION_DIR, extensionFilePath);
}

/**
 * @brief Register a content downloader extension.
 * @param extensionFilePath A full path to an extension to register.
 * @return Returns true if the extension successfully registered.
 */
bool RegisterContentDownloaderExtension(const char* extensionFilePath)
{
    return RegisterExtension(ADUC_CONTENT_DOWNLOADER_EXTENSION_DIR, extensionFilePath);
}

/**
 * @brief Register an extension.
 * @param extensionDir A full path to directory for a generated registration file.
 * @param extensionFilePath A full path to an extension to register.
 * @return Returns true if the extension successfully registered.
 */
bool RegisterExtension(const char* extensionDir, const char* extensionFilePath)
{
    Log_Debug("Registering an extension, target dir: %s, file: %s", extensionDir, extensionFilePath);

    bool success = false;
    char* hash = NULL;
    STRING_HANDLE content = NULL;
    STRING_HANDLE outFilePath = NULL;
    struct passwd* pwd = NULL;
    struct group* grp = NULL;
    FILE* outFile = NULL;

    if (IsNullOrEmpty(extensionDir))
    {
        Log_Error("Invalid target directory.");
        return false;
    }

    if (IsNullOrEmpty(extensionFilePath))
    {
        Log_Error("Invalid extension file path.");
        return false;
    }

    // Note: the return value may point to a static area,
    // and may be overwritten by subsequent calls to getpwent(3), getpwnam(), or getpwuid().
    // (Do not pass the returned pointer to free(3).)
    pwd = getpwnam(ADUC_FILE_USER);
    if (pwd == NULL)
    {
        Log_Error("Cannot verify credential of 'adu' user.");
        goto done;
    }

    uid_t aduUserId = pwd->pw_uid;
    pwd = NULL;

    // Note: The return value may point to a static area,
    // and may be overwritten by subsequent calls to getgrent(3), getgrgid(), or getgrnam().
    // (Do not pass the returned pointer to free(3).)
    grp = getgrnam(ADUC_FILE_GROUP);
    if (grp == NULL)
    {
        Log_Error("Cannot get 'adu' group info.");
        goto done;
    }
    gid_t aduGroupId = grp->gr_gid;
    grp = NULL;

    Log_Debug("Creating the extension folder ('%s'), uid:%d, gid:%d", extensionDir, aduUserId, aduGroupId);
    int dir_result =
        ADUC_SystemUtils_MkDirRecursive(extensionDir, aduUserId, aduGroupId, S_IRWXU | S_IRGRP | S_IWGRP | S_IXGRP);
    if (dir_result != 0)
    {
        Log_Error("Cannot create a folder for registration file. ('%s')", extensionDir);
        goto done;
    }

    struct stat bS;
    if (stat(extensionFilePath, &bS) != 0)
    {
        goto done;
    }

    long fileSize = bS.st_size;

    if (!ADUC_HashUtils_GetFileHash(extensionFilePath, SHA256, &hash))
    {
        goto done;
    }

    content = STRING_construct_sprintf(
        "{\n"
        "   \"fileName\":\"%s\",\n"
        "   \"sizeInBytes\":%d,\n"
        "   \"hashes\": {\n"
        "        \"sha256\":\"%s\"\n"
        "   }\n"
        "}\n",
        extensionFilePath,
        fileSize,
        hash);

    if (content == NULL)
    {
        Log_Error("Cannot construct an extension information.");
        goto done;
    }

    outFilePath = STRING_construct_sprintf("%s/%s", extensionDir, ADUC_EXTENSION_REG_FILENAME);

    outFile = fopen(STRING_c_str(outFilePath), "w");
    if (outFile == NULL)
    {
        Log_Error("Cannot open file: %s", STRING_c_str(outFilePath));
        goto done;
    }

    int ref = fputs(STRING_c_str(content), outFile);
    if (ref < 0)
    {
        Log_Error(
            "Failed to write an extension info to file. File:%s, Content:%s", extensionDir, STRING_c_str(content));
        goto done;
    }

    printf("Successfully registered an extension. Info: %s\n", STRING_c_str(outFilePath));
    success = true;

done:
    if (outFile != NULL)
    {
        fclose(outFile);
    }

    STRING_delete(content);
    STRING_delete(outFilePath);
    free(hash);

    return success;
}
