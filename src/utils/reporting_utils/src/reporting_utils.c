#include "aduc/reporting_utils.h"
#include <stddef.h> // size_t
#include <stdint.h> // int32_t

STRING_HANDLE ADUC_ReportingUtils_CreateReportingErcHexStr(const int32_t erc, bool is_first)
{
    return STRING_construct_sprintf(is_first ? "%08X" : ",%08X", erc);
}

STRING_HANDLE ADUC_ReportingUtils_StringHandleFromVectorInt32(VECTOR_HANDLE vec, size_t max)
{
    bool failed = true;

    size_t vec_size = VECTOR_size(vec);
    vec_size = vec_size < max ? vec_size : max;

    STRING_HANDLE suffix = NULL;
    STRING_HANDLE delimited = STRING_new();
    if (delimited == NULL)
    {
        return NULL;
    }

    for (size_t i = 0; i < vec_size; ++i)
    {
        int concat_result = 0;
        const int32_t* erc = (int32_t*)VECTOR_element(vec, i);

        suffix = ADUC_ReportingUtils_CreateReportingErcHexStr(*erc, false /* is_first */);
        if (suffix == NULL)
        {
            goto done;
        }

        concat_result = STRING_concat_with_STRING(delimited, suffix);
        STRING_delete(suffix);
        suffix = NULL;

        if (concat_result != 0)
        {
            goto done;
        }
    }

    failed = false;
done:

    STRING_delete(suffix);

    if (failed)
    {
        STRING_delete(delimited);
        return NULL;
    }

    return delimited;
}
