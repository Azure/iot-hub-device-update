
#include "root_key_util_helper.h"

#include <parson.h>
#include <stdlib.h>

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
