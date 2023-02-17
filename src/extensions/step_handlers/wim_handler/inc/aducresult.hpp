#ifndef ADUCRESULT_HPP
#define ADUCRESULT_HPP

#include <aduc/result.h> // ADUC_Result_t

/**
 * @class ADUCResult
 * @brief C++ Wrapper around ADUC_Result struct.
 */
class ADUCResult
{
public:
    ADUCResult(ADUC_Result_t ResultCode, ADUC_Result_t ExtendedResultCode = 0)
    {
        Set(ResultCode, ExtendedResultCode);
    }

    ADUCResult(const ADUC_Result& result)
    {
        Set(result.ResultCode, result.ExtendedResultCode);
    }

    operator ADUC_Result() const
    {
        // Return copy of result object.
        return _result;
    }

    void Set(ADUC_Result_t ResultCode, ADUC_Result_t ExtendedResultCode)
    {
        SetResultCode(ResultCode);
        SetExtendedResultcode(ExtendedResultCode);
    }

    ADUC_Result_t ResultCode() const
    {
        return _result.ResultCode;
    }

    void SetResultCode(ADUC_Result_t ResultCode)
    {
        _result.ResultCode = ResultCode;
    }

    ADUC_Result_t ExtendedResultCode() const
    {
        return _result.ExtendedResultCode;
    }

    void SetExtendedResultcode(ADUC_Result_t ExtendedResultCode)
    {
        _result.ExtendedResultCode = ExtendedResultCode;
    }

    bool IsResultCodeSuccess() const
    {
        return IsAducResultCodeSuccess(_result.ResultCode);
    }

    bool IsResultCodeFailure() const
    {
        return IsAducResultCodeFailure(_result.ResultCode);
    }

private:
    ADUC_Result _result;
};

#endif
