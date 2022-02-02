#ifndef EXTENSION_CONTENT_HANDLER_EXPORT_SYMBOLS
#define EXTENSION_CONTENT_HANDLER_EXPORT_SYMBOLS

#include <aduc/exports/extension_common_export_symbols.h> // for GetContractInfo__EXPORT_SYMBOL

//
// Update Content Handler Extension export V1 symbols.
// These are the V1.0 symbols that must be implemented by update content handlers (AKA step handlers).
//

/**
 * @brief GetContractInfo - Gets the extension contract info that the extension implements.
 * @param[out] contractInfo The output parameter the contract info.
 * @returns ADUC_Result The result of the call.
 * @details ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo);
 */
#define CONTENT_HANDLER__GetContractInfo__EXPORT_SYMBOL GetContractInfo__EXPORT_SYMBOL

/**
 * @brief Instantiates an Update Content Handler.
 * @return ContentHandler* The created instance.
 * @details ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel)
 */
#define CONTENT_HANDLER__CreateUpdateContentHandlerExtension__EXPORT_SYMBOL "CreateUpdateContentHandlerExtension"

#endif // EXTENSION_CONTENT_HANDLER_EXPORT_SYMBOLS
