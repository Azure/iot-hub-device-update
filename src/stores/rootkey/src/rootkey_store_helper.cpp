/**
 * @file rootkey_store_helper.cpp
 * @brief Implementation of the root key store helper functions
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "rootkey_store_helper.hpp"
#include <aduc/calloc_wrapper.hpp> // ADUC::StringUtils::cstr_wrapper
#include <aduc/logging.h>
#include <aduc/system_utils.h>
#include <algorithm>
#include <ctime>
#include <parson.h>
#include <sstream>

//
// Internal Implementation static helpers
//
//

static std::string GetTempFilePath()
{
    std::string baseDir{ ADUC_SystemUtils_GetTemporaryPathName() };
    if (baseDir.length() == 0)
    {
        return "";
    }

    std::stringstream ss{ baseDir };

    ss << '/' << std::time(nullptr);
    return ss.str();
}

namespace rootkeystore
{
namespace helper
{

bool IsUpdateStoreNeeded(const std::string& storePath, const std::string& rootKeyPackageJsonString)
{
    bool update_needed = true;
    char* storePackageJsonString = NULL;

    JSON_Value* storePkgJsonValue = json_parse_file(storePath.c_str());

    if (storePkgJsonValue == NULL)
    {
        goto done;
    }

    storePackageJsonString = json_serialize_to_string(storePkgJsonValue);

    if (storePackageJsonString == NULL)
    {
        goto done;
    }

    if (rootKeyPackageJsonString == storePackageJsonString)
    {
        update_needed = false;
    }

done:
    json_free_serialized_string(storePackageJsonString);

    return update_needed;
}

/**
 * @brief Writes the package @p rootKeyPackage to file location @p fileDest atomically
 * @details creates a temp file with the content of @p rootKeyPackage and renames it to @p fileDest
 * @param serializedRootKeyPackage the root key package to be written to the file
 * @param fileDest the destination for the file
 * @return a value of ADUC_Result
 */
ADUC_Result WriteRootKeyPackageToFileAtomically(
    const std::string& serializedRootKeyPackage, const std::string& fileDest)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    JSON_Value* rootKeyPackageValue = NULL;
    std::string tempFilePath;

    if (serializedRootKeyPackage.empty() || fileDest.empty())
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_BAD_ARGS;
        goto done;
    }

    rootKeyPackageValue = json_parse_string(serializedRootKeyPackage.c_str());

    if (rootKeyPackageValue == NULL)
    {
        goto done;
    }

    try
    {
        tempFilePath = std::move(GetTempFilePath());
        if (tempFilePath.length() == 0)
        {
            result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_STORE_TEMPFILENAME;
            goto done;
        }

        if (json_serialize_to_file(rootKeyPackageValue, tempFilePath.c_str()) != JSONSuccess)
        {
            result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_CANNOT_WRITE_PACKAGE_TO_STORE;
            goto done;
        }

        if (rename(tempFilePath.c_str(), fileDest.c_str()) != 0)
        {
            result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_CANT_RENAME_TO_STORE;
            goto done;
        }
    }
    catch(...)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_STORE_TEMPFILENAME;
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;

done:

    if (rootKeyPackageValue != NULL)
    {
        json_value_free(rootKeyPackageValue);
    }

    if (ADUC_SystemUtils_Exists(tempFilePath.c_str()))
    {
        if (remove(tempFilePath.c_str()) != 0)
        {
            Log_Info(
                "Failed to rmv temp file at %s",
                tempFilePath.c_str());
        }
    }

    return result;
}


} // namespace helper
} // namespace rootkeystore
