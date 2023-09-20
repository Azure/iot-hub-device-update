/**
 * @file rootkey_store.cpp
 * @brief Implements the root key store.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "rootkey_store.h"
#include "rootkey_store.hpp"
#include "rootkey_store_helper.hpp"
#include <aduc/calloc_wrapper.hpp> // ADUC::StringUtils::cstr_wrapper
#include <aduc/logging.h>
#include <aduc/rootkeypackage_utils.h>
#include <aduc/string_c_utils.h>
#include <aduc/system_utils.h>
#include <aduc/types/adu_core.h> // ADUC_Result_RootKey_Continue
#include <parson.h>
#include <sys/stat.h> // for stat, struct stat

//
// Public C API
//

EXTERN_C_BEGIN

RootKeyStoreHandle RootKeyStore_CreateInstance()
{
    return (RootKeyStoreHandle)new(std::nothrow) RootKeyStoreInternal;
}

void RootKeyStore_DestroyInstance(RootKeyStoreHandle handle)
{
    if (handle != nullptr)
    {
        delete IRootKeyStoreInternal::store_instance_from_handle(handle);
    }
}

bool RootKeyStore_SetConfig(RootKeyStoreHandle handle, RootKeyStoreConfigProperty configProperty, STRING_HANDLE propertyValue)
{
    if (handle == nullptr || IsNullOrEmpty(STRING_c_str(propertyValue)))
    {
        Log_Error("null/empty args");
        return false;
    }

    try
    {
        IRootKeyStoreInternal::store_instance_from_handle(handle)->SetConfig(configProperty, STRING_c_str(propertyValue));
    }
    catch(...)
    {
        return false;
    }

    return true;
}

STRING_HANDLE RootKeyStore_GetConfig(RootKeyStoreHandle handle, RootKeyStoreConfigProperty configProperty)
{
    const std::string* val = IRootKeyStoreInternal::store_instance_from_handle(handle)->GetConfig(configProperty);
    if (val == nullptr)
    {
        return nullptr;
    }

    return STRING_construct(val->c_str());
}

bool RootKeyStore_GetRootKeyPackage(RootKeyStoreHandle handle, ADUC_RootKeyPackage** outPackage)
{
    if (outPackage == nullptr)
    {
        return false;
    }

    ADUC_RootKeyPackage* pkg = (ADUC_RootKeyPackage*)malloc(sizeof(ADUC_RootKeyPackage));
    if (pkg == NULL)
    {
        return false;
    }
    memset(pkg, 0, sizeof(*pkg));

    bool success = IRootKeyStoreInternal::store_instance_from_handle(handle)->GetRootKeyPackage(pkg);
    if (!success)
    {
        free(pkg);
        pkg = NULL;
        success = true;
    }

    *outPackage = pkg;
    return success;
}

bool RootKeyStore_SetRootKeyPackage(RootKeyStoreHandle handle, const ADUC_RootKeyPackage* package)
{
    if (package == nullptr)
    {
        return false;
    }

    return IRootKeyStoreInternal::store_instance_from_handle(handle)->SetRootKeyPackage(package);
}

bool RootKeyStore_Load(RootKeyStoreHandle handle)
{
    return IRootKeyStoreInternal::store_instance_from_handle(handle)->Load();
}

ADUC_Result RootKeyStore_Persist(RootKeyStoreHandle handle)
{
    return IRootKeyStoreInternal::store_instance_from_handle(handle)->Persist();
}

EXTERN_C_END

//
// Internal Implementation
//

bool RootKeyStoreInternal::SetConfig(RootKeyStoreConfigProperty propertyName, const char* propertyValue) noexcept
{
    switch (propertyName)
    {
    case RootKeyStoreConfig_StorePath:
        try
        {
            m_root_key_path = propertyValue;
        }
        catch(...)
        {
            return false;
        }
    default:
        return false;
    }

    return true;
}

const std::string* RootKeyStoreInternal::GetConfig(RootKeyStoreConfigProperty propertyName) const noexcept
{
    switch (propertyName)
    {
    case RootKeyStoreConfig_StorePath:
        return &m_root_key_path;
    default:
        return nullptr;
    }
}

bool RootKeyStoreInternal::SetRootKeyPackage(const ADUC_RootKeyPackage* package) noexcept
{
    char* json_str = ADUC_RootKeyPackageUtils_SerializePackageToJsonString(package);
    if (json_str == nullptr)
    {
        return false;
    }

    try
    {
        m_serialized_package = json_str;
    }
    catch(...)
    {
        return false;
    }

    return true;
}

bool RootKeyStoreInternal::GetRootKeyPackage(ADUC_RootKeyPackage* outPackage) noexcept
{
    if (outPackage == nullptr || m_root_key_path.length() == 0)
    {
        return false;
    }

    if (m_serialized_package.length() == 0)
    {
        if (!Load())
        {
            return false;
        }
    }

    ADUC_RootKeyPackage tmpPkg;
    memset(&tmpPkg, 0, sizeof(tmpPkg));

    ADUC_Result result = ADUC_RootKeyPackageUtils_Parse(m_serialized_package.c_str(), &tmpPkg);
    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        *outPackage = tmpPkg;
    }
    return IsAducResultCodeSuccess(result.ResultCode);
}

bool RootKeyStoreInternal::Load() noexcept
{
    if (m_root_key_path.length() == 0)
    {
        return false;
    }

    struct stat st = {};
    int exit_code = stat(m_root_key_path.c_str(), &st) == 0;
    if (exit_code != 0)
    {
        Log_Warn("store path stat: %d", exit_code);
        return false;
    }

    JSON_Value* json_value = json_parse_file(m_root_key_path.c_str());
    if (json_value == nullptr)
    {
        return false;
    }

    ADUC::StringUtils::cstr_wrapper serialized{ json_serialize_to_string(json_value) };

    try
    {
        m_serialized_package = serialized.get();
    }
    catch(...)
    {
        return false;
    }

    return true;
}

ADUC_Result RootKeyStoreInternal::Persist() noexcept
{
    ADUC_Result result = { ADUC_GeneralResult_Failure /* ResultCode */, 0 /* ExtendedResultCode */ };

    if (m_root_key_path.length() == 0 || m_serialized_package.length() == 0)
    {
        result.ExtendedResultCode = ADUC_ERC_INVALIDARG;
        return result;
    }

    if (!ADUC_SystemUtils_Exists(m_root_key_path.c_str()))
    {
        if (ADUC_SystemUtils_MkDirRecursiveDefault(m_root_key_path.c_str()) != 0)
        {
            result.ExtendedResultCode = ADUC_ERC_ROOTKEY_STORE_PATH_CREATE;
            return result;
        }
    }

    if (!rootkeystore::helper::IsUpdateStoreNeeded(m_root_key_path, m_serialized_package))
    {
        // This is a success, but skips writing to local store and includes informational ERC.
        result.ResultCode = ADUC_Result_RootKey_Continue;
        result.ExtendedResultCode = ADUC_ERC_ROOTKEY_PKG_UNCHANGED;
        goto done;
    }

    result = rootkeystore::helper::WriteRootKeyPackageToFileAtomically(m_serialized_package, m_root_key_path);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;

done:

    return result;
}
