
/**
 * @file config_parsefile.c
 * @brief Implements parsing the config file
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/config_utils.h"
#include <aduc/c_utils.h>
#include <parson.h>
#include <umock_c/umock_c_prod.h>

/**
 * @brief Takes in the config file path and parse it into a JSON Value with Parson
 *
 * @param configFilePath
 * @return JSON_Value*
 */
JSON_Value* Parse_JSON_File(const char* configFilePath)
{
    return json_parse_file(configFilePath);
}
