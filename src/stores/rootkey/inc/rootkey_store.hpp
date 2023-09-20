/**
 * @file rootkey_store.cpp
 * @brief Implements the root key package store.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "rootkey_store_types.h"
#include <aduc/result.h>
#include <aduc/rootkeypackage_types.h> // for ADUC_RootKeyPackage
#include <string>

class IRootKeyStoreInternal
{
public:
    virtual bool SetConfig(RootKeyStoreConfigProperty propertyName, const char* propertyValue) noexcept = 0;
    virtual const std::string* GetConfig(RootKeyStoreConfigProperty propertyName) const noexcept = 0;
    virtual bool SetRootKeyPackage(const ADUC_RootKeyPackage* package) noexcept = 0;
    virtual bool GetRootKeyPackage(ADUC_RootKeyPackage* outPackage) noexcept = 0;
    virtual bool Load() noexcept = 0;
    virtual ADUC_Result Persist() noexcept = 0;

    static IRootKeyStoreInternal* store_instance_from_handle(RootKeyStoreHandle h) noexcept
    {
        return reinterpret_cast<IRootKeyStoreInternal*>(h);
    }
};

class RootKeyStoreInternal : public IRootKeyStoreInternal
{
public:
    RootKeyStoreInternal() {}
    bool SetConfig(RootKeyStoreConfigProperty propertyName, const char* propertyValue) noexcept override;
    const std::string* GetConfig(RootKeyStoreConfigProperty propertyName) const noexcept override;
    bool SetRootKeyPackage(const ADUC_RootKeyPackage* package) noexcept override;
    bool GetRootKeyPackage(ADUC_RootKeyPackage* outPackage) noexcept override;
    bool Load() noexcept override;
    ADUC_Result Persist() noexcept override;

private:
    std::string m_root_key_path;
    std::string m_serialized_package;
    ADUC_Result_t m_extended_result_code;
};
