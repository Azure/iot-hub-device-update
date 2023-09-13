#ifndef ROOT_KEY_UTILS_HELPER
#define ROOT_KEY_UTILS_HELPER

#include <umock_c/umock_c_prod.h>

#ifdef ENABLE_MOCKS
#    undef ENABLE_MOCKS
#    include "aduc/rootkeypackage_types.h"
#    define ENABLE_MOCKS
#else
#    include "aduc/rootkeypackage_types.h"
#endif

#include <aduc/result.h>

MOCKABLE_FUNCTION(
    , bool, RootKeyUtility_RootKeyIsDisabled, const ADUC_RootKeyPackage*, rootKeyPackage, const char*, keyId);

MOCKABLE_FUNCTION(
    ,
    ADUC_Result,
    RootKeyUtility_LoadPackageFromDisk,
    ADUC_RootKeyPackage**,
    rootKeyPackage,
    const char*,
    fileLocation);

#endif // #define ROOT_KEY_UTILS_HELPER
