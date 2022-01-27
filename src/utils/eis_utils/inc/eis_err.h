/**
 * @file eis_err.h
 * @brief Header file defining the errors which can occur within the Edge Identity Service (EIS) Utility
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/c_utils.h>

#ifndef EIS_ERR_H
#    define EIS_ERR_H

EXTERN_C_BEGIN

/**
 * @brief Error codes for failures which could occur within the EIS Utility and associated calls.
 */
typedef enum tagEISErr
{
    EISErr_Failed = 0, /**< Failed*/
    EISErr_Ok = 1, /**< No error encountered */
    EISErr_InvalidArg = 2, /**< Invalid Argument was passed to the EIS Utility*/
    EISErr_ConnErr = 3, /**< Unable to connect to the service */
    EISErr_TimeoutErr = 4, /**< Request timed out */
    EISErr_HTTPErr = 5, /**< An HTTP error code was returned during a request */
    EISErr_RecvInvalidValueErr = 6, /**< Service returned an invalid value */
    EISErr_RecvRespOutOfLimitsErr = 7, /**< Service returned a response out of the EIS Utility Limits */
    EISErr_ContentAllocErr = 8, /**< EisComs could not allocate enough memory for the content*/
    EISErr_InvalidJsonRespErr = 9, /**< Service returned invalid JSON */
} EISErr;

/**
 * @brief Service which encountered the respective error
 */
typedef enum tagEISService
{
    EISService_Utils = 0, /**< Error occurred within the EIS Utility itself */
    EISService_IdentityService = 1, /**< Error occurred within a call to the Identity Service */
    EISService_KeyService = 2, /**< Error occurred within the Key Service */
    EISService_CertService = 3, /**< Error occurred within the Certificate Service */
} EISService;

/**
 * @brief Returns the string that matches the @p eisErr value
 * @param eisErr error for which
 * @returns the string name of the EISErr or "<Unknown>"
 */
const char* EISErr_ErrToString(EISErr eisErr);

/**
 * @brief Returns the string that matches the @p eisService value
 * @param eisService service to get the string for
 * @returns the string name of the EISService or "<Unknown>"
 */
const char* EISService_ServiceToString(EISService eisService);

/**
 * @brief Return value for any EIS Utility
 */
typedef struct tagEISUtilityResult
{
    EISErr err; /**Error that occurred */
    EISService service; /**Service on which the error occurred */
} EISUtilityResult;

EXTERN_C_END

#endif
