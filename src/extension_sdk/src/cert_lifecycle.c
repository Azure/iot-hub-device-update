/**
 * @file cert_lifecycle.c
 * @brief Certificate management for mTLS connections.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef _WIN32
#define _GNU_SOURCE
#endif

#include "aduc/cert_lifecycle.h"

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define CERT_ERR_INVALID_ARG   ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_AUTH, 0x0010)
#define CERT_ERR_FILE_OPEN     ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_IO, 0x0010)
#define CERT_ERR_PARSE         ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_IO, 0x0011)
#define CERT_ERR_CHAIN_INVALID ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_AUTH, 0x0011)
#define CERT_ERR_STORE         ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_RESOURCE, 0x0010)

static X509* load_cert_from_file(const char* path)
{
    FILE* f = fopen(path, "r");
    if (f == NULL)
    {
        return NULL;
    }
    X509* cert = PEM_read_X509(f, NULL, NULL, NULL);
    fclose(f);
    return cert;
}

static void extract_cn(X509_NAME* name, char* buf, size_t bufLen)
{
    buf[0] = '\0';
    if (name == NULL)
    {
        return;
    }
    int idx = X509_NAME_get_index_by_NID(name, NID_commonName, -1);
    if (idx < 0)
    {
        return;
    }
    X509_NAME_ENTRY* entry = X509_NAME_get_entry(name, idx);
    if (entry == NULL)
    {
        return;
    }
    ASN1_STRING* data = X509_NAME_ENTRY_get_data(entry);
    if (data == NULL)
    {
        return;
    }
    unsigned char* utf8 = NULL;
    int len = ASN1_STRING_to_UTF8(&utf8, data);
    if (len > 0 && utf8 != NULL)
    {
        size_t copyLen = (size_t)len < bufLen - 1 ? (size_t)len : bufLen - 1;
        memcpy(buf, utf8, copyLen);
        buf[copyLen] = '\0';
        OPENSSL_free(utf8);
    }
}

static uint64_t asn1_time_to_epoch(const ASN1_TIME* t)
{
    struct tm tm_val;
    memset(&tm_val, 0, sizeof(tm_val));

    if (ASN1_TIME_to_tm(t, &tm_val) != 1)
    {
        return 0;
    }

    // Convert struct tm to time_t (UTC)
    time_t epoch = timegm(&tm_val);
    return epoch > 0 ? (uint64_t)epoch : 0;
}

ADUC_Result2 ADUC_Cert_GetInfo(const char* certPath, ADUC_CertInfo* outInfo)
{
    if (certPath == NULL || outInfo == NULL)
    {
        return CERT_ERR_INVALID_ARG;
    }

    memset(outInfo, 0, sizeof(*outInfo));

    X509* cert = load_cert_from_file(certPath);
    if (cert == NULL)
    {
        return CERT_ERR_FILE_OPEN;
    }

    // Extract subject CN
    extract_cn(X509_get_subject_name(cert), outInfo->subjectCN, sizeof(outInfo->subjectCN));

    // Extract issuer CN
    extract_cn(X509_get_issuer_name(cert), outInfo->issuerCN, sizeof(outInfo->issuerCN));

    // Extract serial number
    const ASN1_INTEGER* serial = X509_get0_serialNumber(cert);
    if (serial != NULL)
    {
        BIGNUM* bn = ASN1_INTEGER_to_BN(serial, NULL);
        if (bn != NULL)
        {
            char* hex = BN_bn2hex(bn);
            if (hex != NULL)
            {
                strncpy(outInfo->serialNumber, hex, sizeof(outInfo->serialNumber) - 1);
                OPENSSL_free(hex);
            }
            BN_free(bn);
        }
    }

    // Extract validity
    const ASN1_TIME* notBefore = X509_get0_notBefore(cert);
    const ASN1_TIME* notAfter = X509_get0_notAfter(cert);

    outInfo->notBefore = asn1_time_to_epoch(notBefore);
    outInfo->notAfter = asn1_time_to_epoch(notAfter);

    // Check expiry
    time_t now = time(NULL);
    if ((uint64_t)now >= outInfo->notAfter)
    {
        outInfo->isExpired = true;
        outInfo->daysUntilExpiry = 0;
    }
    else
    {
        outInfo->isExpired = false;
        uint64_t remaining = outInfo->notAfter - (uint64_t)now;
        outInfo->daysUntilExpiry = (uint32_t)(remaining / 86400);
    }

    X509_free(cert);
    return ADUC_RESULT2_SUCCESS;
}

bool ADUC_Cert_NeedsRenewal(const ADUC_CertInfo* info, uint32_t thresholdDays)
{
    if (info == NULL)
    {
        return true;
    }
    if (info->isExpired)
    {
        return true;
    }
    return info->daysUntilExpiry <= thresholdDays;
}

ADUC_Result2 ADUC_Cert_ValidateChain(const char* certPath, const char* caPath)
{
    if (certPath == NULL || caPath == NULL)
    {
        return CERT_ERR_INVALID_ARG;
    }

    X509* cert = load_cert_from_file(certPath);
    if (cert == NULL)
    {
        return CERT_ERR_FILE_OPEN;
    }

    X509_STORE* store = X509_STORE_new();
    if (store == NULL)
    {
        X509_free(cert);
        return CERT_ERR_STORE;
    }

    if (X509_STORE_load_locations(store, caPath, NULL) != 1)
    {
        X509_STORE_free(store);
        X509_free(cert);
        return CERT_ERR_FILE_OPEN;
    }

    X509_STORE_CTX* ctx = X509_STORE_CTX_new();
    if (ctx == NULL)
    {
        X509_STORE_free(store);
        X509_free(cert);
        return CERT_ERR_STORE;
    }

    if (X509_STORE_CTX_init(ctx, store, cert, NULL) != 1)
    {
        X509_STORE_CTX_free(ctx);
        X509_STORE_free(store);
        X509_free(cert);
        return CERT_ERR_STORE;
    }

    int verifyResult = X509_verify_cert(ctx);

    X509_STORE_CTX_free(ctx);
    X509_STORE_free(store);
    X509_free(cert);

    if (verifyResult != 1)
    {
        return CERT_ERR_CHAIN_INVALID;
    }

    return ADUC_RESULT2_SUCCESS;
}
