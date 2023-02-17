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

#define ADUC_V1_CONTRACT_MAJOR_VER 1
#define ADUC_V1_CONTRACT_MINOR_VER 0

typedef struct tagADUC_ExtensionContractInfo
{
    unsigned int majorVer;
    unsigned int minorVer;
} ADUC_ExtensionContractInfo;

EXTERN_C_BEGIN

/**
 * @brief Returns whether the contract info is a V1 contract.
 *
 * @param contractInfo The contract info.
 * @return true when contractInfo is NULL or is 1.0
 */
bool ADUC_ContractUtils_IsV1Contract(ADUC_ExtensionContractInfo* contractInfo);

/**
 * @brief Returns whether the contract info has version greater than or equal to the given major and minor versions.
 *
 * @param contractInfo The contract info.
 * @param majorVer The major version.
 * @param minorVer The minor version.
 * @return true if the version is greater than or equal to that in the contract info. If contractInfo is NULL, it is treated as version 1.0
 */
bool ADUC_ContractUtils_IsVersionGTE(
    ADUC_ExtensionContractInfo* contractInfo, unsigned int majorVer, unsigned int minorVer);

EXTERN_C_END

#endif // ADUC_CONTRACT_UTILS
