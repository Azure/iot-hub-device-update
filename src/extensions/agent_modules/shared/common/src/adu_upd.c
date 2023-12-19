#include "aduc/adu_upd.h"

/**
 * @brief Gets the string representation of the update module state.
 *
 * @param updateModuleState The update module state.
 * @return const char* The string form of the update module state.
 */
const char* AduUpdState_str(ADU_UPD_STATE updateModuleState)
{
    switch (updateModuleState)
    {
    case ADU_UPD_STATE_READY:
        return "Ready";

    case ADU_UPD_STATE_IDLEWAIT:
        return "Idle Wait";

    case ADU_UPD_STATE_REQUESTING:
        return "Requesting";

    case ADU_UPD_STATE_RETRYWAIT:
        return "Retry Wait";

    case ADU_UPD_STATE_REQUEST_ACK:
        return "Request Acknowledged";

    case ADU_UPD_STATE_PROCESSING_UPDATE:
        return "Processing Update";

    case ADU_UPD_STATE_REPORT_RESULTS:
        return "ADU_UPD_STATE_REPORT_RESULTS";

    case ADU_UPD_STATE_REPORT_RESULTS_ACK:
        return "ADU_UPD_STATE_REPORT_RESULTS_ACK";

    default:
        return "???";
    }
}
