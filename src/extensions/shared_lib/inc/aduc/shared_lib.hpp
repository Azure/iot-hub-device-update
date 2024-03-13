/**
 * @file shared_lib.hpp
 * @brief header for aduc::shared_lib class.
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_SHARED_LIB
#define ADUC_SHARED_LIB

#include <string>
#include <vector>

namespace aduc
{
class SharedLib
{
public:
    /**
     * @brief Default constructor for SharedLib
     * @param libPath the path to the library
     */
    SharedLib(const std::string& libPath);
    /**
     * @brief Default destructor for the SharedLib object
     */
    ~SharedLib();
    /**
     * @brief Ensure's the symbols within @p symbols
     * @param symbols a vector of symbol strings
     */
    void EnsureSymbols(std::vector<std::string> symbols) const;
    /**
     * @brief Gets the symbols from @p symbol and returns them in bin format
     * @param symbol the symbol to export
     * @returns the bin version of symbol
     */
    void* GetSymbol(const std::string& symbol) const;

private:
    void* libHandle; // !< The library handle the SharedLib is representing
};

} // namespace aduc

#endif // ADUC_SHARED_LIB
