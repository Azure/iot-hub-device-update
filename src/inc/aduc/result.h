/**
 * @file result.h
 * @brief Describes the ADUC result type.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#ifndef ADUC_RESULT_H
#define ADUC_RESULT_H

#include <stdbool.h> // _Bool
#include <stdint.h> // int32_t
/**
 * @brief Defines the type of an ADUC_Result.
 */
typedef int32_t ADUC_Result_t;

/**
 * @brief Defines an ADUC_Result object which is used to indicate status.
 */
typedef struct tagADUC_Result
{
    ADUC_Result_t ResultCode; /**< Method-specific result. Value > 0 indicates success. */
    ADUC_Result_t ExtendedResultCode; /**< Implementation-specific extended result code. */
} ADUC_Result;

/**
 * @brief Determines if a result code is succeeded.
 */
static inline _Bool IsAducResultCodeSuccess(const ADUC_Result_t resultCode)
{
    return (resultCode > 0);
}

/**
 * @brief Determines if a result code is failed.
 */
static inline _Bool IsAducResultCodeFailure(const ADUC_Result_t resultCode)
{
    return (resultCode <= 0);
}

/**
 * @brief Facility codes to pass to MAKE_ADUC_EXTENDEDRESULTCODE.
 */
typedef enum tagADUC_Facility
{
    /*indicates errors from SWUPDATE Handler. */
    ADUC_FACILITY_SWUPDATE_HANDLER = 0x1,
    /*indicates errors from PVCONTROL Handler. */
    ADUC_FACILITY_PVCONTROL_HANDLER = 0x2,
    /*indicates errors from APT Handler. */
    ADUC_FACILITY_APT_HANDLER = 0xA,
    /*indicates errors from cryptographic validation*/
    ADUC_FACILITY_CRYPTO = 0xC,
    /*indicates errors from Delivery Optimization downloader. */
    ADUC_FACILITY_DELIVERY_OPTIMIZATION = 0xD,
    /*indicates errno errors. */
    ADUC_FACILITY_ERRNO = 0xE,
    /*indicates errors from the lower layer*/
    ADUC_FACILITY_LOWERLAYER = 0x10,
} ADUC_Facility;

/**
 * @brief Converts an error to a 32-bit value extended result code.  Top 4 bits is facility, rest is value.
 */
static inline ADUC_Result_t MAKE_ADUC_EXTENDEDRESULTCODE(const ADUC_Facility facility, const unsigned int value)
{
    return ((facility & 0xF) << 0x1C) | (value & 0xFFFFFFF);
}

/**
 * @brief Macros to convert APT Handler results to extended result code values.
 */
static inline ADUC_Result_t MAKE_ADUC_APT_HANDLER_EXTENDEDRESULTCODE(const int32_t value)
{
    return MAKE_ADUC_EXTENDEDRESULTCODE(ADUC_FACILITY_APT_HANDLER, value);
}

/**
 * @brief Macros to convert SWUPDATE Handler results to extended result code values.
 */
static inline ADUC_Result_t MAKE_ADUC_SWUPDATE_HANDLER_EXTENDEDRESULTCODE(const int32_t value)
{
    return MAKE_ADUC_EXTENDEDRESULTCODE(ADUC_FACILITY_SWUPDATE_HANDLER, value);
}

/**
 * @brief Macros to convert PVCONTROL Handler results to extended result code values.
 */
static inline ADUC_Result_t MAKE_ADUC_PVCONTROL_HANDLER_EXTENDEDRESULTCODE(const int32_t value)
{
    return MAKE_ADUC_EXTENDEDRESULTCODE(ADUC_FACILITY_PVCONTROL_HANDLER, value);
}

/**
 * @brief Macros to convert Delivery Optimization results to extended result code values.
 */
static inline ADUC_Result_t MAKE_ADUC_DELIVERY_OPTIMIZATION_EXTENDEDRESULTCODE(const int32_t value)
{
    return MAKE_ADUC_EXTENDEDRESULTCODE(ADUC_FACILITY_DELIVERY_OPTIMIZATION, value);
}

/**
 * @brief Macros to convert errno values to extended result code values.
 */
static inline ADUC_Result_t MAKE_ADUC_ERRNO_EXTENDEDRESULTCODE(const int value)
{
    return MAKE_ADUC_EXTENDEDRESULTCODE(ADUC_FACILITY_ERRNO, value);
}

/**
 * @brief Macros to convert cryptographic validation results to extended result code values
 */
static inline ADUC_Result_t MAKE_ADUC_VALIDATION_EXTENDEDRESULTCODE(const int value)
{
    return MAKE_ADUC_EXTENDEDRESULTCODE(ADUC_FACILITY_CRYPTO, value);
}

/**
 * @brief Macros to convert json validation results to extended result code values
 */
static inline ADUC_Result_t MAKE_ADUC_LOWERLAYER_EXTENDEDRESULTCODE(const int value)
{
    return MAKE_ADUC_EXTENDEDRESULTCODE(ADUC_FACILITY_LOWERLAYER, value);
}

// Note: POSIX 2001 Standard ErrNos
// Errno Values taken from /usr/include/asm-generic/errno-base.h
// and                     /usr/include/asm-generic/errno.h

#define ADUC_ERC_NOTRECOVERABLE MAKE_ADUC_ERRNO_EXTENDEDRESULTCODE(131)
#define ADUC_ERC_NOMEM MAKE_ADUC_ERRNO_EXTENDEDRESULTCODE(12)
#define ADUC_ERC_NOTPERMITTED MAKE_ADUC_ERRNO_EXTENDEDRESULTCODE(1)

#define ADUC_ERC_APT_HANDLER_ERROR_NONE MAKE_ADUC_APT_HANDLER_EXTENDEDRESULTCODE(0)
#define ADUC_ERC_APT_HANDLER_INITIALIZATION_FAILURE MAKE_ADUC_APT_HANDLER_EXTENDEDRESULTCODE(1)
#define ADUC_ERC_APT_HANDLER_INVALID_PACKAGE_DATA MAKE_ADUC_APT_HANDLER_EXTENDEDRESULTCODE(2)
#define ADUC_ERC_APT_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_VERSION MAKE_ADUC_APT_HANDLER_EXTENDEDRESULTCODE(3)
#define ADUC_ERC_APT_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_FILECOUNT MAKE_ADUC_APT_HANDLER_EXTENDEDRESULTCODE(4)
#define ADUC_ERC_APT_HANDLER_PACKAGE_DOWNLOAD_FAILURE MAKE_ADUC_APT_HANDLER_EXTENDEDRESULTCODE(5)
#define ADUC_ERC_APT_HANDLER_PACKAGE_INSTALL_FAILURE MAKE_ADUC_APT_HANDLER_EXTENDEDRESULTCODE(6)
#define ADUC_ERC_APT_HANDLER_PACKAGE_CANCEL_FAILURE MAKE_ADUC_APT_HANDLER_EXTENDEDRESULTCODE(7)
#define ADUC_ERC_APT_HANDLER_INSTALLCRITERIA_PERSIST_FAILURE MAKE_ADUC_APT_HANDLER_EXTENDEDRESULTCODE(8)

#define ADUC_ERC_SWUPDATE_HANDLER_EMPTY_VERSION MAKE_ADUC_SWUPDATE_HANDLER_EXTENDEDRESULTCODE(0)
#define ADUC_ERC_SWUPDATE_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_VERSION \
    MAKE_ADUC_SWUPDATE_HANDLER_EXTENDEDRESULTCODE(1)
#define ADUC_ERC_SWUPDATE_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_FILECOUNT \
    MAKE_ADUC_SWUPDATE_HANDLER_EXTENDEDRESULTCODE(2)

#define ADUC_ERC_PVCONTROL_HANDLER_EMPTY_VERSION MAKE_ADUC_PVCONTROL_HANDLER_EXTENDEDRESULTCODE(0)
#define ADUC_ERC_PVCONTROL_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_VERSION \
    MAKE_ADUC_PVCONTROL_HANDLER_EXTENDEDRESULTCODE(1)
#define ADUC_ERC_PVCONTROL_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_FILECOUNT \
    MAKE_ADUC_PVCONTROL_HANDLER_EXTENDEDRESULTCODE(2)

#define ADUC_ERC_VALIDATION_FILE_HASH_IS_EMPTY MAKE_ADUC_VALIDATION_EXTENDEDRESULTCODE(1)
#define ADUC_ERC_VALIDATION_FILE_HASH_TYPE_NOT_SUPPORTED MAKE_ADUC_VALIDATION_EXTENDEDRESULTCODE(2)
#define ADUC_ERC_VALIDATION_FILE_HASH_INVALID_HASH MAKE_ADUC_VALIDATION_EXTENDEDRESULTCODE(3)

#define ADUC_ERC_LOWERLEVEL_INVALID_UPDATE_ACTION MAKE_ADUC_LOWERLAYER_EXTENDEDRESULTCODE(1)
#define ADUC_ERC_LOWERLEVEL_UPDATE_MANIFEST_VALIDATION_INVALID_HASH MAKE_ADUC_VALIDATION_EXTENDEDRESULTCODE(2)

#endif // ADUC_RESULT_H
