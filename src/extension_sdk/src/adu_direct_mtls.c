/**
 * @file adu_direct_mtls.c
 * @brief Client mTLS configuration helper implementation.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_direct_mtls.h"
#include "aduc/extension_types.h"
#include "aduc/platform.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <toml.h>

ADUC_Result2 ADUC_Mtls_ApplyToCurl(void* curlHandle, const ADUC_MtlsConfig* config)
{
    if (curlHandle == NULL || config == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_CONFIG, 1);
    }

    CURL* curl = (CURL*)curlHandle;

    if (config->certPath[0] != '\0')
    {
        curl_easy_setopt(curl, CURLOPT_SSLCERT, config->certPath);
    }

    if (config->keyPath[0] != '\0')
    {
        curl_easy_setopt(curl, CURLOPT_SSLKEY, config->keyPath);
    }

    if (config->caPath[0] != '\0')
    {
        curl_easy_setopt(curl, CURLOPT_CAINFO, config->caPath);
    }

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, config->verifyPeer ? 1L : 0L);

    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_Mtls_LoadConfig(const char* configSection, ADUC_MtlsConfig* outConfig)
{
    if (configSection == NULL || outConfig == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_CONFIG, 2);
    }

    memset(outConfig, 0, sizeof(*outConfig));
    outConfig->verifyPeer = true;

    /* Attempt to open and parse the agent config file */
    FILE* fp = fopen("/etc/adu/adu-agent.toml", "r");
    if (fp == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_IO, 1);
    }

    char errbuf[200];
    toml_table_t* root = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (root == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_CONFIG, 3);
    }

    toml_table_t* section = toml_table_in(root, configSection);
    if (section == NULL)
    {
        toml_free(root);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_CONFIG, 4);
    }

    toml_datum_t certPath = toml_string_in(section, "cert_path");
    if (certPath.ok)
    {
        strncpy(outConfig->certPath, certPath.u.s, sizeof(outConfig->certPath) - 1);
        free(certPath.u.s);
    }

    toml_datum_t keyPath = toml_string_in(section, "key_path");
    if (keyPath.ok)
    {
        strncpy(outConfig->keyPath, keyPath.u.s, sizeof(outConfig->keyPath) - 1);
        free(keyPath.u.s);
    }

    toml_datum_t caPath = toml_string_in(section, "ca_path");
    if (caPath.ok)
    {
        strncpy(outConfig->caPath, caPath.u.s, sizeof(outConfig->caPath) - 1);
        free(caPath.u.s);
    }

    toml_datum_t verifyPeer = toml_bool_in(section, "verify_peer");
    if (verifyPeer.ok)
    {
        outConfig->verifyPeer = (bool)verifyPeer.u.b;
    }

    toml_free(root);
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_Mtls_ValidateFiles(const ADUC_MtlsConfig* config)
{
    if (config == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_CONFIG, 1);
    }

    if (config->certPath[0] != '\0')
    {
        if (access(config->certPath, R_OK) != 0)
        {
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_IO, 2);
        }
    }

    if (config->keyPath[0] != '\0')
    {
        if (access(config->keyPath, R_OK) != 0)
        {
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_IO, 3);
        }
    }

    if (config->caPath[0] != '\0')
    {
        if (access(config->caPath, R_OK) != 0)
        {
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_IO, 4);
        }
    }

    return ADUC_RESULT2_SUCCESS;
}
