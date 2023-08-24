/**
 * @file contract_utils.h
 * @brief The header for contract utils.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_CONTRACT_UTILS
#define ADUC_CONTRACT_UTILS

#include <aduc/c_utils.h>
#include <aduc/result.h>
#include <stdbool.h>

#define ADUC_V1_CONTRACT_MAJOR_VER 1 //!< The major version of the v1 contract model
#define ADUC_V1_CONTRACT_MINOR_VER 0 //!< The minor version of the v1 contract model

/**
 * @brief The Extenstion Contract Info struct that wraps the version for the contract information
 */
typedef struct tagADUC_ExtensionContractInfo
{
    unsigned int majorVer; //!< The major version of the contract
    unsigned int minorVer; //!< The minor version of the contract
} ADUC_ExtensionContractInfo;

EXTERN_C_BEGIN

/**
 * @brief Checks if @p contractInfo is a v1 contract
 * @param contractInfo the contractInfo to check
 * @returns true if a v1 contract version ; false otherwise
 */
bool ADUC_ContractUtils_IsV1Contract(ADUC_ExtensionContractInfo* contractInfo);

EXTERN_C_END

#endif // ADUC_CONTRACT_UTILS
