/**
 * @file extension_common_export_symbols.h
 * @brief The common function export symbols used by specific extension export symbols headers.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef EXTENSION_COMMON_EXPORT_SYMBOLS_H
#define EXTENSION_COMMON_EXPORT_SYMBOLS_H

//
// Contract Info Symbols
// Any symbols in this section will exist in shared library of each extension
// type.
//
// It is only optional in pre-V1 legacy code.
// All extension implementations going forward should have this symbol
// available to be called.
//
// This symbol macro should not be used directly but referred to by
// other per-extension macros below, typically always the first one per
// extension type.
//

//
// Version 1.0 Signatures and associated function export symbols
//

/**
 * @brief GetContractInfo - Gets the extension contract info that the extension implements.
 * @param[out] contractInfo The output parameter the contract info.
 * @returns ADUC_Result The result of the call.
 * @details ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo);
 */
#define GetContractInfo__EXPORT_SYMBOL "GetContractInfo"

#endif // EXTENSION_COMMON_EXPORT_SYMBOLS_H
