/**
 * @file eis_err_ut.cpp
 * @brief Unit Tests for eis_err (EIS error/service enum-to-string utilities)
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch_all.hpp>
#include "eis_err.h"
#include <cstring>

TEST_CASE("EISErr_ErrToString - All error codes")
{
    SECTION("EISErr_Ok returns correct string")
    {
        const char* result = EISErr_ErrToString(EISErr_Ok);
        CHECK(strcmp(result, "EISErr_Ok") == 0);
    }
    
    SECTION("EISErr_Failed returns correct string")
    {
        const char* result = EISErr_ErrToString(EISErr_Failed);
        CHECK(strcmp(result, "EISErr_Failed") == 0);
    }
    
    SECTION("EISErr_InvalidArg returns correct string")
    {
        const char* result = EISErr_ErrToString(EISErr_InvalidArg);
        CHECK(strcmp(result, "EISErr_InvalidArg") == 0);
    }
    
    SECTION("EISErr_ConnErr returns correct string")
    {
        const char* result = EISErr_ErrToString(EISErr_ConnErr);
        CHECK(strcmp(result, "EISErr_ConnErr") == 0);
    }
    
    SECTION("EISErr_TimeoutErr returns correct string")
    {
        const char* result = EISErr_ErrToString(EISErr_TimeoutErr);
        CHECK(strcmp(result, "EISErr_TimeoutErr") == 0);
    }
    
    SECTION("EISErr_HTTPErr returns correct string")
    {
        const char* result = EISErr_ErrToString(EISErr_HTTPErr);
        CHECK(strcmp(result, "EISErr_HTTPErr") == 0);
    }
    
    SECTION("EISErr_RecvInvalidValueErr returns correct string")
    {
        const char* result = EISErr_ErrToString(EISErr_RecvInvalidValueErr);
        CHECK(strcmp(result, "EISErr_RecvInvalidValueErr") == 0);
    }
    
    SECTION("EISErr_RecvRespOutOfLimitsErr returns correct string")
    {
        const char* result = EISErr_ErrToString(EISErr_RecvRespOutOfLimitsErr);
        CHECK(strcmp(result, "EISErr_RecvRespOutOfLimitsErr") == 0);
    }
    
    SECTION("EISErr_ContentAllocErr returns correct string")
    {
        const char* result = EISErr_ErrToString(EISErr_ContentAllocErr);
        CHECK(strcmp(result, "EISErr_ContentAllocErr") == 0);
    }
    
    SECTION("EISErr_InvalidJsonRespErr returns correct string")
    {
        const char* result = EISErr_ErrToString(EISErr_InvalidJsonRespErr);
        CHECK(strcmp(result, "EISErr_InvalidJsonRespErr") == 0);
    }
    
    SECTION("Invalid error code returns <Unknown>")
    {
        const char* result = EISErr_ErrToString(static_cast<EISErr>(9999));
        CHECK(strcmp(result, "<Unknown>") == 0);
    }
}

TEST_CASE("EISService_ServiceToString - All service codes")
{
    SECTION("EISService_Utils returns correct string")
    {
        const char* result = EISService_ServiceToString(EISService_Utils);
        CHECK(strcmp(result, "EISService_Utils") == 0);
    }
    
    SECTION("EISService_IdentityService returns correct string")
    {
        const char* result = EISService_ServiceToString(EISService_IdentityService);
        CHECK(strcmp(result, "EISService_IdentityService") == 0);
    }
    
    SECTION("EISService_KeyService returns correct string")
    {
        const char* result = EISService_ServiceToString(EISService_KeyService);
        CHECK(strcmp(result, "EISService_KeyService") == 0);
    }
    
    SECTION("EISService_CertService returns correct string")
    {
        const char* result = EISService_ServiceToString(EISService_CertService);
        CHECK(strcmp(result, "EISService_CertService") == 0);
    }
    
    SECTION("Invalid service code returns <Unknown>")
    {
        const char* result = EISService_ServiceToString(static_cast<EISService>(9999));
        CHECK(strcmp(result, "<Unknown>") == 0);
    }
}

TEST_CASE("EISUtilityResult - Structure usage")
{
    SECTION("Can create and access EISUtilityResult structure")
    {
        EISUtilityResult result;
        result.err = EISErr_Ok;
        result.service = EISService_IdentityService;
        
        CHECK(result.err == EISErr_Ok);
        CHECK(result.service == EISService_IdentityService);
        
        // Verify string conversion works with struct members
        CHECK(strcmp(EISErr_ErrToString(result.err), "EISErr_Ok") == 0);
        CHECK(strcmp(EISService_ServiceToString(result.service), "EISService_IdentityService") == 0);
    }
    
    SECTION("Can create error result")
    {
        EISUtilityResult result;
        result.err = EISErr_HTTPErr;
        result.service = EISService_KeyService;
        
        CHECK(result.err == EISErr_HTTPErr);
        CHECK(strcmp(EISErr_ErrToString(result.err), "EISErr_HTTPErr") == 0);
    }
}