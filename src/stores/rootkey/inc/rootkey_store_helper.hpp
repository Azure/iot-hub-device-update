/**
 * @file rootkey_store_helper.hpp
 * @brief The header interface for the rootkey store helper functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ROOTKEY_STORE_HELPER_HPP
#define ROOTKEY_STORE_HELPER_HPP

#include <aduc/result.h>
#include <string>

namespace rootkeystore
{
namespace helper
{

bool IsUpdateStoreNeeded(const std::string& storePath, const std::string& rootKeyPackageJsonString);
ADUC_Result WriteRootKeyPackageToFileAtomically(const std::string& serializedRootKeyPackage, const std::string& fileDest);

} // namespace helper
} // namespace rootkeystore

#endif // #define ROOTKEY_STORE_HELPER_HPP
