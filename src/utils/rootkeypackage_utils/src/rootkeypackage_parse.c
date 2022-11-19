
#include "aduc/rootkeypackage_parse.h"

EXTERN_C_BEGIN

/**
 * @brief Parses the version protected property in accordance with rootkeypackage.schema.json
 *
 * @param rootValue The root JSON value.
 * @param[out] outPackage The root key package object to write parsed version data.
 *
 * @return true on successful parse.
 */
_Bool RootKeyPackage_ParseVersion(JSON_Value* rootValue, ADUC_RootKeyPackage* outPackage)
{
    _Bool parsed = false;

    return parsed;
}

/**
 * @brief Parses the published protected property in accordance with rootkeypackage.schema.json
 *
 * @param rootValue The root JSON value.
 * @param[out] outPackage The root key package object to write parsed published data.
 *
 * @return true on successful parse.
 */
_Bool RootKeyPackage_ParsePublished(JSON_Value* rootValue, ADUC_RootKeyPackage* outPackage)
{
    _Bool parsed = false;

    return parsed;
}

/**
 * @brief Parses the disabledRootKeys protected property in accordance with rootkeypackage.schema.json
 *
 * @param rootValue The root JSON value.
 * @param[out] outPackage The root key package object to write parsed disabledRootKeys data.
 *
 * @return true on successful parse.
 */
_Bool RootKeyPackage_ParseDisabledRootKeys(JSON_Value* rootValue, ADUC_RootKeyPackage* outPackage)
{
    _Bool parsed = false;

    return parsed;
}

/**
 * @brief Parses the disabledSigningKeys protected property in accordance with rootkeypackage.schema.json
 *
 * @param rootValue The root JSON value.
 * @param[out] outPackage The root key package object to write parsed disabledSigningKeys data.
 *
 * @return true on successful parse.
 */
_Bool RootKeyPackage_ParseDisabledSigningKeys(JSON_Value* rootValue, ADUC_RootKeyPackage* outPackage)
{
    _Bool parsed = false;

    return parsed;
}

/**
 * @brief Parses the rootKeys protected property in accordance with rootkeypackage.schema.json
 *
 * @param rootValue The root JSON value.
 * @param[out] outPackage The root key package object to write parsed rootKeys data.
 *
 * @return true on successful parse.
 */
_Bool RootKeyPackage_ParseRootKeys(JSON_Value* rootValue, ADUC_RootKeyPackage* outPackage)
{
    _Bool parsed = false;

    return parsed;
}

/**
 * @brief Parses the protected properties in accordance with rootkeypackage.schema.json
 *
 * @param rootValue The root JSON value.
 * @param[out] outPackage The root key package object to write parsed protected properties data.
 *
 * @return true on successful parse.
 */
_Bool RootKeyPackage_ParseProtectedProperties(JSON_Value* rootValue, ADUC_RootKeyPackage* outPackage)
{
    _Bool parsed = false;

    if (!(parsed = RootKeyPackage_ParseVersion(rootValue, outPackage)))
    {
        goto done;
    }

    if (!(parsed = RootKeyPackage_ParsePublished(rootValue, outPackage)))
    {
        goto done;
    }

    if (!(parsed = RootKeyPackage_ParseDisabledRootKeys(rootValue, outPackage)))
    {
        goto done;
    }

    if (!(parsed = RootKeyPackage_ParseDisabledSigningKeys(rootValue, outPackage)))
    {
        goto done;
    }

    if (!(parsed = RootKeyPackage_ParseRootKeys(rootValue, outPackage)))
    {
        goto done;
    }

    parsed = true;
done:

    return parsed;
}

/**
 * @brief Parses the signatures properties in accordance with rootkeypackage.schema.json
 *
 * @param rootValue The root JSON value.
 * @param[out] outPackage The root key package object to write parsed signatures data.
 *
 * @return true on successful parse.
 */
_Bool RootKeyPackage_ParseSignatures(JSON_Value* rootValue, ADUC_RootKeyPackage* outPackage)
{
    _Bool parsed = false;

    return parsed;
}

EXTERN_C_END
