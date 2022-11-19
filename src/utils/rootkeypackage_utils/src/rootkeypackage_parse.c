
#include "aduc/rootkeypackage_parse.h"

EXTERN_C_BEGIN

/**
 * @brief Parses the version protected property in accordance with rootkeypackage.schema.json
 *
 * @param protectedPropertiesObj The root JSON object.
 * @param[out] outPackage The root key package object to write parsed version data.
 *
 * @return true on successful parse.
 */
_Bool RootKeyPackage_ParseVersion(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage)
{
    unsigned long version = 0;

    if (protectedPropertiesObj == NULL || outPackage == NULL)
    {
        return false;
    }

    version = json_object_get_number(protectedPropertiesObj, "version");
    if (version == 0)
    {
        return false;
    }

    (outPackage->protectedProperties).version = version;
    return true;
}

/**
 * @brief Parses the published protected property in accordance with rootkeypackage.schema.json
 *
 * @param protectedPropertiesObj The protected properties JSON object.
 * @param[out] outPackage The root key package object to write parsed published data.
 *
 * @return true on successful parse.
 */
_Bool RootKeyPackage_ParsePublished(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage)
{
    time_t published = 0;
    if (protectedPropertiesObj == NULL || outPackage == NULL)
    {
        return false;
    }

    published = json_object_get_number(protectedPropertiesObj, "version");
    if (published <= 0)
    {
        return false;
    }

    (outPackage->protectedProperties).publishedTime = published;
    return true;
}

/**
 * @brief Parses the disabledRootKeys protected property in accordance with rootkeypackage.schema.json
 *
 * @param protectedPropertiesObj The protected properties JSON object.
 * @param[out] outPackage The root key package object to write parsed disabledRootKeys data.
 *
 * @return true on successful parse.
 */
_Bool RootKeyPackage_ParseDisabledRootKeys(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage)
{
    _Bool parsed = false;

    return parsed;
}

/**
 * @brief Parses the disabledSigningKeys protected property in accordance with rootkeypackage.schema.json
 *
 * @param protectedPropertiesObj The protected properties JSON object.
 * @param[out] outPackage The root key package object to write parsed disabledSigningKeys data.
 *
 * @return true on successful parse.
 */
_Bool RootKeyPackage_ParseDisabledSigningKeys(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage)
{
    _Bool parsed = false;

    return parsed;
}

/**
 * @brief Parses the rootKeys protected property in accordance with rootkeypackage.schema.json
 *
 * @param protectedPropertiesObj The protected properties JSON object.
 * @param[out] outPackage The root key package object to write parsed rootKeys data.
 *
 * @return true on successful parse.
 */
_Bool RootKeyPackage_ParseRootKeys(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage)
{
    _Bool parsed = false;

    return parsed;
}

/**
 * @brief Parses the protected properties in accordance with rootkeypackage.schema.json
 *
 * @param protectedPropertiesObj The protected properties JSON object.
 * @param[out] outPackage The root key package object to write parsed protected properties data.
 *
 * @return true on successful parse.
 */
_Bool RootKeyPackage_ParseProtectedProperties(JSON_Object* rootObj, ADUC_RootKeyPackage* outPackage)
{
    _Bool parsed = false;

    JSON_Object* protectedPropertiesObj = json_object_get_object(rootObj, "protected");
    if (protectedPropertiesObj == NULL)
    {
        goto done;
    }

    if (!(parsed = RootKeyPackage_ParseVersion(protectedPropertiesObj, outPackage)))
    {
        goto done;
    }

    if (!(parsed = RootKeyPackage_ParsePublished(protectedPropertiesObj, outPackage)))
    {
        goto done;
    }

    if (!(parsed = RootKeyPackage_ParseDisabledRootKeys(protectedPropertiesObj, outPackage)))
    {
        goto done;
    }

    if (!(parsed = RootKeyPackage_ParseDisabledSigningKeys(protectedPropertiesObj, outPackage)))
    {
        goto done;
    }

    if (!(parsed = RootKeyPackage_ParseRootKeys(protectedPropertiesObj, outPackage)))
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
 * @param rootObj The root JSON object.
 * @param[out] outPackage The root key package object to write parsed signatures data.
 *
 * @return true on successful parse.
 */
_Bool RootKeyPackage_ParseSignatures(JSON_Object* rootObj, ADUC_RootKeyPackage* outPackage)
{
    _Bool parsed = false;

    JSON_Array* signaturesArray = json_object_get_array(rootObj, "signatures");
    if (signaturesArray == NULL)
    {
        goto done;
    }

    parsed = true;
done:

    return parsed;
}

EXTERN_C_END
