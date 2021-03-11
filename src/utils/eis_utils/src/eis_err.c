/**
 * @file eis_err.c
 * @brief Implementation file for the EISErrors with helpers
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "eis_err.h"

/**
 * @brief Returns the string that matches the @p eisService value
 * @param eisService service to get the string for
 * @returns the string name of the EISService or "<Unknown>"
 */
const char* EISService_ServiceToString(EISService eisService)
{
    switch (eisService)
    {
    case EISService_Utils:
        return "EISService_Utils";
    case EISService_IdentityService:
        return "EISService_IdentityService";
    case EISService_KeyService:
        return "EISService_KeyService";
    case EISService_CertService:
        return "EISService_CertService";
    }
    return "<Unknown>";
}

/**
 * @brief Returns the string that matches the @p eisErr value
 * @param eisErr error to get the string for
 * @returns the string name of the EISErr or "<Unknown>"
 */
const char* EISErr_ErrToString(EISErr eisErr)
{
    switch (eisErr)
    {
    case EISErr_Failed:
        return "EISErr_Failed";
    case EISErr_Ok:
        return "EISErr_Ok";
    case EISErr_InvalidArg:
        return "EISErr_InvalidArg";
    case EISErr_ConnErr:
        return "EISErr_ConnErr";
    case EISErr_TimeoutErr:
        return "EISErr_TimeoutErr";
    case EISErr_HTTPErr:
        return "EISErr_HTTPErr";
    case EISErr_RecvInvalidValueErr:
        return "EISErr_RecvInvalidValueErr";
    case EISErr_RecvRespOutOfLimitsErr:
        return "EISErr_RecvRespOutOfLimitsErr";
    case EISErr_ContentAllocErr:
        return "EISErr_ContentAllocErr";
    case EISErr_InvalidJsonRespErr:
        return "EISErr_InvalidJsonRespErr";
    }
    return "<Unknown>";
}
