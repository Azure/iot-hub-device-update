/**
 * @file plugin_call_helper.hpp
 * @brief header for helper functions when calling plugin export functions.
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef PLUGIN_CALL_HELPER
#define PLUGIN_CALL_HELPER

#include "aduc/plugin_exception.hpp"
#include "aduc/shared_lib.hpp"
#include <aduc/result.h>

template<int v>
struct IntToType
{
    enum
    {
        value = v
    };
};

/**
 * @brief Calls the resolved symbol that does not return an ADUC_Result.
 *
 * @tparam FunctionSignature The function signature of the resolved function symbol.
 * @tparam Arguments The arguments to pass to the function. Omit if none.
 * @param resolvedSym The resolved symbol
 * @param outResult unused.
 * @param args The arguments to pass to the export function.
 */
template<typename FunctionSignature, typename... Arguments>
void CallExportHandlerInternal(IntToType<false>, void* resolvedSym, ADUC_Result* outResult, Arguments... args)
{
    UNREFERENCED_PARAMETER(outResult);

    auto fn = reinterpret_cast<FunctionSignature>(resolvedSym);
    fn(args...);
}

/**
 * @brief Calls the resolved symbol and outputs the resulting ADUC_Result.
 *
 * @tparam FunctionSignature The function signature of the resolved function symbol.
 * @tparam Arguments The arguments to pass to the function. Omit if none.
 * @param resolvedSym The resolved symbol
 * @param[out] outResult The out result to set with the results of export function call.
 * @param args The arguments to pass to the export function.
 */
template<typename FunctionSignature, typename... Arguments>
void CallExportHandlerInternal(IntToType<true>, void* resolvedSym, ADUC_Result* outResult, Arguments... args)
{
    auto fn = reinterpret_cast<FunctionSignature>(resolvedSym);
    *outResult = fn(args...);
}

/**
 * @brief Casts the resolved symbol to provided compile-time function signature and calls it wit hthe provided arguments.
 * Sets the outResult with resulting ADUC_Result if ExportReturnsAducResult Boolean template parameter is set to true and
 * outResult is not nullptr at runtime.
 *
 * @tparam FunctionSignature The function signature of the resolved function symbol.
 * @tparam ExportReturnsAducResult Whether the function signature returns an
 * ADUC_Result or not. If it does, then it will set the contents of outResult
 * at runtime if outResult is not nullptr.
 * @tparam Arguments The arguments to pass to the function. Omit if none.
 * @param resolvedSymbol The resolved symbol.
 * @param[out] outResult Optional. The resulting ADUC_Result from calling the export function.
 * @param args The arguments to pass to the export function.
 */
template<typename FunctionSignature, bool ExportReturnsAducResult, typename... Arguments>
void CallExportHandler(void* resolvedSymbol, ADUC_Result* outResult, Arguments... args)
{
    CallExportHandlerInternal<FunctionSignature>(
        IntToType<ExportReturnsAducResult>(), resolvedSymbol, outResult, args...);
}

/**
 * @brief Calls an export function public symbol on a shared library with
 * compile-time verified FunctionSignature of the export function and
 * compile-time switching based on if the export returns and ADUC_Result or not.
 *
 * @tparam FunctionSignature The function signature.
 * @tparam ExportReturnsAducResult Whether the function signature returns an
 * ADUC_Result or not. If it does, then it will set the contents of outResult
 * at runtime.
 * @tparam Arguments The arguments to pass to the function. Omit if none.
 * @param exportSymbol The public export symbol to call on the shared library.
 * @param lib The shared library.
 * @param[out] outResult Optional. The ADUC_Result out to set if
 * ExportReturnsAducResult is true.
 * @param args The variadic arguments to pass on to the invoked export function.
 * @details Can throw aduc::PluginException if it is unable to resolve an export symbol.
 */
template<typename FunctionSignature, bool ExportReturnsAducResult, typename... Arguments>
void CallExport(const char* exportSymbol, const aduc::SharedLib& lib, ADUC_Result* outResult, Arguments... args)
{
    void* resolvedSymbol = nullptr;

    try
    {
        Log_Debug("Looking up symbol '%s'", exportSymbol);
        resolvedSymbol = lib.GetSymbol(exportSymbol);
    }
    catch (const std::exception& ex)
    {
        Log_Error("Exception calling symbol '%s': %s", exportSymbol, ex.what());
    }
    catch (...)
    {
        Log_Error("Non std exception when calling symbol '%s'.", exportSymbol);
    }

    if (resolvedSymbol == nullptr)
    {
        Log_Error("Could not resolve export symbol '%s'", exportSymbol);
        throw aduc::PluginException("unresolved symbol", exportSymbol);
    }

    try
    {
        CallExportHandler<FunctionSignature, ExportReturnsAducResult>(resolvedSymbol, outResult, args...);
    }
    catch (const std::exception& ex)
    {
        Log_Error("Exception calling symbol '%s': %s", exportSymbol, ex.what());
    }
    catch (...)
    {
        Log_Error("Non std exception when calling symbol '%s'.", exportSymbol);
    }
}

#endif // PLUGIN_CALL_HELPER
