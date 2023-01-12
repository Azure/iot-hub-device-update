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

bool ADUC_ContractUtils_IsV1Contract(ADUC_ExtensionContractInfo* contractInfo);

EXTERN_C_END

#endif // ADUC_CONTRACT_UTILS
