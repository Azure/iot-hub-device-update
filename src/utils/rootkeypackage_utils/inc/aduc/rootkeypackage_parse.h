#ifndef ROOTKEYPACKAGE_PARSE_H
#define ROOTKEYPACKAGE_PARSE_H

#include "aduc/rootkeypackage_types.h"
#include <aduc/c_utils.h>
#include <parson.h>
#include <stdbool.h>

EXTERN_C_BEGIN

_Bool RootKeyPackage_ParseVersion(JSON_Value* rootValue, ADUC_RootKeyPackage* outPackage);
_Bool RootKeyPackage_ParsePublished(JSON_Value* rootValue, ADUC_RootKeyPackage* outPackage);
_Bool RootKeyPackage_ParseDisabledRootKeys(JSON_Value* rootValue, ADUC_RootKeyPackage* outPackage);
_Bool RootKeyPackage_ParseDisabledSigningKeys(JSON_Value* rootValue, ADUC_RootKeyPackage* outPackage);
_Bool RootKeyPackage_ParseRootKeys(JSON_Value* rootValue, ADUC_RootKeyPackage* outPackage);
_Bool RootKeyPackage_ParseProtectedProperties(JSON_Value* rootValue, ADUC_RootKeyPackage* outPackage);
_Bool RootKeyPackage_ParseSignatures(JSON_Value* rootValue, ADUC_RootKeyPackage* outPackage);

EXTERN_C_END

#endif // ROOTKEYPACKAGE_PARSE_H
