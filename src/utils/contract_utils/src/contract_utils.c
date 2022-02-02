/**
 * @file contract_utils.c
 * @brief The implementation for contract utils.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/contract_utils.h"
#include <aduc/result.h>
#include <stddef.h>

bool ADUC_ContractUtils_IsV1Contract(ADUC_ExtensionContractInfo* contractInfo)
{
    // V1 contract includes extensions that have no GetContractInfo symbol (NULL contractInfo argument)
    // or are explicitly set to 1.0
    return contractInfo == NULL
        || (contractInfo->majorVer == ADUC_V1_CONTRACT_MAJOR_VER
            && contractInfo->minorVer == ADUC_V1_CONTRACT_MINOR_VER);
}
