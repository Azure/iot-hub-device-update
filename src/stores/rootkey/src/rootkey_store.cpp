/**
 * @file rootkey_store.cpp
 * @brief Implements the root key store.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "rootkey_store.h"
#include "rootkey_store.hpp"
#include <parson.h>

//
// Public C API
//

EXTERN_C_BEGIN

RootKeyStoreHandle RootKeyStore_CreateInstance()
{
    return (RootKeyStoreHandle)new(std::noexcept) RootKeyStoreInternal;
}

void RootKeyStore_DestroyInstance(RootKeyStoreHandle handle)
{
    if (handle != nullptr)
    {
        delete RootKeyStoreInternal::store_instance_from_handle(handle);
    }
}

bool RootKeyStore_SetConfig(RootKeyStoreHandle handle, RootKeyStoreConfigProperty configProperty, STRING_HANDLE propertyValue)
{
    if (handle == nullptr || IsNullOrEmpty(STRING_c_str(propertyValue)))
    {
        return false;
    }

    try
    {
        RootKeyStoreInternal::store_instance_from_handle(handle)->SetConfig(configProperty, STRING_c_str(propertyValue));
    }
    catch(...)
    {
        return false;
    }

    return true;
}

STRING_HANDLE RootKeyStore_GetConfig(RootKeyStoreHandle handle, RootKeyStoreConfigProperty configProperty)
{
    const std::string* val = RootKeyStoreInternal::store_instance_from_handle(handle)->GetConfig(configProperty);
    if (val == nullptr)
    {
        return nullptr;
    }

    return STRING_construct(val->c_str());
}

bool RootKeyStore_GetRootKeyPackage(RootKeyStoreHandle handle, ADUC_RootKeyPackage* outPackage)
{
    if (outPackage == nullptr)
    {
        return false;
    }

    return RootKeyStoreInternal::store_instance_from_handle(handle)->GetRootKeyPackage(outPackage);
}

bool RootKeyStore_SetRootKeyPackage(RootKeyStoreHandle handle, const ADUC_RootKeyPackage* package)
{
    if (package == nullptr)
    {
        return false;
    }

    return RootKeyStoreInternal::store_instance_from_handle(handle)->SetRootKeyPackage(package);
}

bool RootKeyStore_Load(RootKeyStoreHandle handle)
{
    return RootKeyStoreInternal::store_instance_from_handle(handle)->Load();
}

bool RootKeyStore_Persist(RootKeyStoreHandle handle)
{
    return RootKeyStoreInternal::store_instance_from_handle(handle)->Persist();
}

EXTERN_C_END

//
// Internal Implementation
//

void RootKeyStoreInternal::SetConfig(RootKeyStoreConfigProperty propertyName, const char* propertyValue) noexcept
{
    switch (configProperty)
    {
    case RootKeyStoreConfigProperty.RootKeyStoreConfig_StorePath:
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
    switch (configProperty)
    {
    case RootKeyStoreConfigProperty.RootKeyStoreConfig_StorePath:
        return &m_root_key_path;
    default:
        return nullptr;
    }
}

bool RootKeyStoreInternal::SetRootKeyPackage(const ADUC_RootKeyPackage* package) noexcept
{
    char* json_str = ADUC_RootKeyPackageUtils_SerializePackageToJsonString(package);
    if (json_str == null_ptr)
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

bool RootKeyStoreInternal::GetRootKeyPackage(ADUC_RootKeyPackage* outPackage) const noexcept
{
    if (m_serialized_package.length() == 0)
    {
        return nullptr;
    }

    return IsAducResultCodeSuccess(ADUC_RootKeyPackageUtils_Parse(m_serialized_package.c_str(), outPackage));
}

bool RootKeyStoreInternal::Load() noexcept
{
    if (m_root_key_path.length() == 0)
    {
        return false;
    }

    JSON_Value* json_value = json_parse_file(m_root_key_path);
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

bool RootKeyStoreInternal::Persist() noexcept
{
    if (m_root_key_path.length() == 0 || m_serialized_package.length() == 0)
    {
        return false;
    }

    JSON_Value* json_value = json_parse_string(m_serialized_package)
    if (json_value == nullptr)
    {
        return false;
    }

    auto json_result = json_serialize_to_file(json_value, m_root_key_path);
    json_value_free(json_value);

    return json_result == JSON_Success;
}
