
#include "root_key_util_helper.h"

#include <parson.h>
#include <stdlib.h>

// Forward declarations
ADUC_Result RootKeyUtility_LoadSerializedPackage(const char* fileLocation, char** outSerializePackage);
ADUC_Result ADUC_RootKeyPackageUtils_Parse(const char* jsonString, ADUC_RootKeyPackage* outRootKeyPackage);
ADUC_Result RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(const ADUC_RootKeyPackage* rootKeyPackage);
void ADUC_RootKeyPackageUtils_Destroy(ADUC_RootKeyPackage* rootKeyPackage);

/**
 * @brief Checks if the rootkey represented by @p keyId is in the disabledRootKeys of @p rootKeyPackage
 *
 * @param keyId the id of the key to check
 * @return true if the key is in the disabledKeys section, false if it isn't
 */
bool RootKeyUtility_RootKeyIsDisabled(const ADUC_RootKeyPackage* rootKeyPackage, const char* keyId)
{
    if (rootKeyPackage == NULL)
    {
        return true;
    }

    const size_t numDisabledKeys = VECTOR_size(rootKeyPackage->protectedProperties.disabledRootKeys);

    for (size_t i = 0; i < numDisabledKeys; ++i)
    {
        STRING_HANDLE* disabledKey = VECTOR_element(rootKeyPackage->protectedProperties.disabledRootKeys, i);

        if (strcmp(STRING_c_str(*disabledKey), keyId) == 0)
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Loads the RootKeyPackage from disk at file location @p fileLocation
 *
 * @param rootKeyPackage the address of the pointer to the root key package to be set
 * @param fileLocation the file location to read the data from
 * @return a value of ADUC_Result
 */
ADUC_Result RootKeyUtility_LoadPackageFromDisk(ADUC_RootKeyPackage** rootKeyPackage, const char* fileLocation)
{
    // ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    // ADUC_Result tmpResult = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    ADUC_Result result = { .ResultCode = 2, .ExtendedResultCode = 0 };
    ADUC_Result tmpResult = { .ResultCode = 2, .ExtendedResultCode = 0 };
    JSON_Value* rootKeyPackageValue = NULL;

    ADUC_RootKeyPackage* tempPkg = NULL;

    char* rootKeyPackageJsonString = NULL;

    if (fileLocation == NULL || rootKeyPackage == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_BAD_ARGS;
        goto done;
    }

    tmpResult = RootKeyUtility_LoadSerializedPackage(fileLocation, &rootKeyPackageJsonString);
    if (IsAducResultCodeFailure(tmpResult.ResultCode))
    {
        result = tmpResult;
        goto done;
    }

    tempPkg = (ADUC_RootKeyPackage*)malloc(sizeof(ADUC_RootKeyPackage));

    if (tempPkg == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_ERRNOMEM;
        goto done;
    }

    ADUC_Result parseResult = ADUC_RootKeyPackageUtils_Parse(rootKeyPackageJsonString, tempPkg);

    if (IsAducResultCodeFailure(parseResult.ResultCode))
    {
        result = parseResult;
        goto done;
    }

    ADUC_Result validationResult = RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(tempPkg);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        result = validationResult;
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        ADUC_RootKeyPackageUtils_Destroy(tempPkg);
        free(tempPkg);
        tempPkg = NULL;
    }

    if (rootKeyPackageValue != NULL)
    {
        json_value_free(rootKeyPackageValue);
    }

    free(rootKeyPackageJsonString);

    *rootKeyPackage = tempPkg;

    result.ResultCode = 7;
    return result;
}
