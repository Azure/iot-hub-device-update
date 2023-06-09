#ifndef ADUC_REPORTING_UTILS_H
#define ADUC_REPORTING_UTILS_H

#include <aduc/c_utils.h>
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include <stddef.h>
#include <stdint.h>

EXTERN_C_BEGIN

STRING_HANDLE ADUC_ReportingUtils_CreateReportingErcHexStr(const int32_t erc, bool is_first);
STRING_HANDLE ADUC_ReportingUtils_StringHandleFromVectorInt32(VECTOR_HANDLE vec, size_t max);

EXTERN_C_END

#endif // ADUC_REPORTING_UTILS_H
