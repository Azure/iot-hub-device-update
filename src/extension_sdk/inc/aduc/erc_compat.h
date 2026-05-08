/**
 * @file erc_compat.h
 * @brief Maps Gen2 structured result codes to Gen1 Extended Result Codes (ERC).
 *
 * Gen1 ERC format: 0x{component:4}{facility:4}{code:24}
 * Gen1 components: 0x1=Agent, 0x2=DO(Delivery Optimization), 0x3=Extension, 0x5=Upper
 *
 * This header provides inline mapping for backward-compatible reporting to
 * the Device Update service, which expects Gen1-style 32-bit ERC values.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_ERC_COMPAT_H
#define ADUC_ERC_COMPAT_H

#include "aduc/result_codes.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Gen1 component identifiers (top nibble of ERC).
 */
#define ADUC_ERC_COMPONENT_AGENT     0x1
#define ADUC_ERC_COMPONENT_DO        0x2
#define ADUC_ERC_COMPONENT_EXTENSION 0x3
#define ADUC_ERC_COMPONENT_UPPER     0x5

/**
 * @brief Map a Gen2 ADUC_Result2 to a Gen1-compatible Extended Result Code.
 *
 * Gen1 ERC layout: [component:4][facility:4][category:8][specific:16]
 *
 * @param rc  The Gen2 structured result code.
 * @return    A 32-bit Gen1-compatible ERC value.
 */
static inline int32_t ADUC_MapToExtendedResultCode(ADUC_Result2 rc)
{
    if (ADUC_RESULT2_IS_SUCCESS(rc))
    {
        return 0;
    }

    uint8_t facility = ADUC_RC_GET_FACILITY(rc);
    uint8_t category = ADUC_RC_GET_CATEGORY(rc);
    uint16_t specific = ADUC_RC_GET_SPECIFIC(rc);

    /* Map Gen2 facility to Gen1 component */
    uint8_t component = ADUC_ERC_COMPONENT_AGENT; /* default */
    switch (facility)
    {
        case ADUC_FACILITY_DOWNLOAD:
            component = ADUC_ERC_COMPONENT_DO;
            break;
        case ADUC_FACILITY_EXTENSION:
            component = ADUC_ERC_COMPONENT_EXTENSION;
            break;
        case ADUC_FACILITY_WORKFLOW:
            component = ADUC_ERC_COMPONENT_UPPER;
            break;
        default:
            /* AGENT, COMM, SECURITY all map to Agent component */
            component = ADUC_ERC_COMPONENT_AGENT;
            break;
    }

    return (int32_t)(
        ((uint32_t)component << 28) |
        ((uint32_t)facility << 24) |
        ((uint32_t)category << 16) |
        (uint32_t)specific);
}

/**
 * @brief Reconstruct a Gen2 ADUC_Result2 from a Gen1-style ERC.
 *
 * Extracts the facility, category, and specific fields from the ERC layout.
 * The component nibble is discarded (it is redundant with facility).
 *
 * @param erc  A Gen1-compatible 32-bit ERC value.
 * @return     The corresponding Gen2 ADUC_Result2.
 */
static inline ADUC_Result2 ADUC_MapFromExtendedResultCode(int32_t erc)
{
    if (erc == 0)
    {
        return ADUC_RESULT2_SUCCESS;
    }

    uint32_t uerc = (uint32_t)erc;
    uint8_t facility = (uint8_t)((uerc >> 24) & 0x0F);
    uint8_t category = (uint8_t)((uerc >> 16) & 0xFF);
    uint16_t specific = (uint16_t)(uerc & 0xFFFF);

    return ADUC_RESULT2_MAKE(facility, category, specific);
}

#ifdef __cplusplus
}
#endif

#endif /* ADUC_ERC_COMPAT_H */
