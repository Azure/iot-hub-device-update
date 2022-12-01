/**
 * @file shared_lib.cpp
 * @brief Implementation for utility class that wraps loading a dynamic shared library and allows ensuring/getting symbols.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/shared_lib.hpp"
#include "aduc/hash_utils.h"

#include <algorithm> // std::for_each

#if defined(_WIN32)
// TODO(JeffMill): [PAL] dlopen, dlerror, dlsym, dlclose
#    define RTLD_LAZY 0

void* dlopen(const char* filename, int flag)
{
    return NULL;
}

char* dlerror(void)
{
    return "NYI";
}

void* dlsym(void* handle, const char* symbol)
{
    return NULL;
}

int dlclose(void* handle)
{
    return 0;
}
#else
#    include <dlfcn.h> // dlopen
#endif

#include <stdexcept> // std::runtime_error

namespace aduc
{
SharedLib::SharedLib(const std::string& libPath)
{
    dlerror(); // clear
    void* handle = dlopen(libPath.c_str(), RTLD_LAZY);
    if (nullptr == handle)
    {
        char* error = dlerror();
        throw std::runtime_error(error != nullptr ? error : "dlopen");
    }

    libHandle = handle;
}

SharedLib::~SharedLib()
{
    if (libHandle != nullptr)
    {
        dlclose(libHandle);
        libHandle = nullptr;
    }
}

void SharedLib::EnsureSymbols(std::vector<std::string> symbols) const
{
    std::for_each(symbols.begin(), symbols.end(), [this](const std::string& sym) { GetSymbol(sym); });
}

void* SharedLib::GetSymbol(const std::string& symbol) const
{
    dlerror(); // clear
    void* sym = dlsym(libHandle, symbol.c_str());
    if (sym == nullptr)
    {
        char* error = dlerror();
        throw std::runtime_error(error != nullptr ? error : "dlsym");
    }

    return sym;
}

} // namespace aduc
