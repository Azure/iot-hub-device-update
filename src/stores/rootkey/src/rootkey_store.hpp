/**
 * @file rootkey_store.cpp
 * @brief Implements the root key store.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "rootkey_store_types.h"
#include <aduc/rootkeypackage_types.h> // for ADUC_RootKeyPackage
#include <string>

class RootKeyStoreInternal
{
public:
    RootKeyStoreInternal = default;
    ~RootKeyStoreInternal = default;
    void SetConfig(RootKeyStoreConfigProperty propertyName, std::string propertyValue) noexcept;
    const std::string* GetConfig(RootKeyStoreConfigProperty propertyName) const noexcept;
    bool SetRootKeyPackage(const ADUC_RootKeyPackage* package) noexcept;
    bool GetRootKeyPackage(ADUC_RootKeyPackage* outPackage) const noexcept;
    bool Load() noexcept;
    bool Persist() noexcept;

    static RootKeyStoreInternal* store_instance_from_handle(RootKeyStoreHandle h) noexcept
    {
        return reinterpret_cast<RootKeyStoreInstaler*>(h);
    }

private:
    std::string m_root_key_path;
    std::string m_serialized_package;
};
