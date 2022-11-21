#ifndef ROOTKEYPACKAGE_PARSE_H
#define ROOTKEYPACKAGE_PARSE_H

#include "aduc/rootkeypackage_types.h"
#include <aduc/c_utils.h>
#include <aduc/result.h>
#include <parson.h>
#include <stdbool.h>

EXTERN_C_BEGIN

ADUC_Result RootKeyPackage_ParseVersion(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage);
ADUC_Result RootKeyPackage_ParsePublished(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage);
ADUC_Result RootKeyPackage_ParseDisabledRootKeys(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage);
ADUC_Result RootKeyPackage_ParseHashAlg(JSON_Object* jsonObj, SHAversion* outAlg);
ADUC_Result RootKeyPackage_ParseSigningAlg(JSON_Object* jsonObj, ADUC_RootKeySigningAlgorithm* outAlg);
ADUC_Result
RootKeyPackage_ParseDisabledSigningKeys(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage);
ADUC_Result RootKeyPackage_ParseRootKeys(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage);
ADUC_Result
RootKeyPackage_ParseProtectedProperties(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage);
ADUC_Result RootKeyPackage_ParseSignatures(JSON_Object* signaturesObj, ADUC_RootKeyPackage* outPackage);

void ADUC_RootKey_DeInit(ADUC_RootKey* node);
void ADUC_RootKeyPackage_Hash_DeInit(ADUC_RootKeyPackage_Hash* node);
void ADUC_RootKeyPackage_Signature_DeInit(ADUC_RootKeyPackage_Signature* node);

EXTERN_C_END

#endif // ROOTKEYPACKAGE_PARSE_H