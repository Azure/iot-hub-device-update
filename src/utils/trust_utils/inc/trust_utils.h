#ifndef TRUST_UTILS_H
#define TRUST_UTILS_H

#include <aduc/result.h>
#include <parson.h>

ADUC_Result validate_update_manifest_signature(JSON_Object* updateActionObject);

#endif // #define TRUST_UTILS_H
