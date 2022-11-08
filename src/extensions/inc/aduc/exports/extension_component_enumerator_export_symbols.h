/**
 * @file extension_component_enumerator_export_symbols.h
 * @brief The function export symbols for component enumerator extensions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef EXTENSION_COMPONENT_ENUMERATOR_EXPORT_SYMBOLS_H
#define EXTENSION_COMPONENT_ENUMERATOR_EXPORT_SYMBOLS_H

#include <aduc/exports/extension_common_export_symbols.h> // for GetContractInfo__EXPORT_SYMBOL

//
// Component Enumerator Extension export V1 symbols.
// These are the V1 symbols that must be implemented by Component Enumerator Extensions.
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
#define COMPONENT_ENUMERATOR__GetContractInfo__EXPORT_SYMBOL GetContractInfo__EXPORT_SYMBOL

/**
 * @brief Returns all components information in JSON format.
 * @param includeProperties Indicates whether to include optional component's properties in the output string.
 * @return Returns a serialized json data contains components information. Caller must call FreeComponentsDataString function
 * when done with the returned string.
 * @details char* GetAllComponents()
 */
#define COMPONENT_ENUMERATOR__GetAllComponents__EXPORT_SYMBOL "GetAllComponents"

/**
 * @brief Select component(s) that contain property or properties matching specified in @p selectorJson string.
 *
 * Example input json:
 *      - Select all components belong to a 'Motors' group
 *              "{\"group\":\"Motors\"}"
 *
 *      - Select a component with name equals 'left-motor'
 *              "{\"name\":\"left-motor\"}"
 *
 *      - Select components matching specified class (manufature/model)
 *              "{\"manufacturer\":\"Contoso\",\"model\":\"USB-Motor-0001\"}"
 *
 * @param selectorJson A stringified json containing one or more properties use for components selection.
 * @return Returns a serialized json data containing components information.
 * Caller must call FreeString function when done with the returned string.
 * @details char* SelectComponents(const char* selectorJson)
 */
#define COMPONENT_ENUMERATOR__SelectComponents__EXPORT_SYMBOL "SelectComponents"

/**
 * @brief Frees the components data string allocated by GetAllComponents.
 *
 * @param string The string previously returned by GetAllComponents.
 * @details void FreeComponentsDataString(char* string)
 */
#define COMPONENT_ENUMERATOR__FreeComponentsDataString__EXPORT_SYMBOL "FreeComponentsDataString"

#endif // EXTENSION_COMPONENT_ENUMERATOR_EXPORT_SYMBOLS_H
