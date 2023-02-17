#ifndef WIM_HANDLER_RESULT_CODE_HPP
#define WIM_HANDLER_RESULT_CODE_HPP

#include "aduc/types/adu_core.h" // ADUC_Result_*
#include "aducresult.hpp" // ADUCResult

enum class WimHandlerResultCode
{
    Success = 0,

    General_CantGetWorkFolder = 100,

    Manifest_CantGetFileEntity = 200,
    Manifest_WrongFileCount,
    Manifest_UnsupportedUpdateVersion,
    Manifest_MissingInstalledCriteria,

    Download_UnknownException = 300,

    Install_UnknownException = 400,

    Apply_UnknownException = 500,
};

class WimHandlerADUCResult : public ADUCResult
{
public:
    WimHandlerADUCResult(WimHandlerResultCode code) : ADUCResult(ADUC_Result_Failure, static_cast<ADUC_Result_t>(code))
    {
        if (code == WimHandlerResultCode::Success)
        {
            Set(ADUC_Result_Success, 0);
        }
    }
};

#endif
