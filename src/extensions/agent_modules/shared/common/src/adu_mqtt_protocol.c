#include "aduc/adu_mqtt_protocol.h"

/**
 * @brief Get the str form of the result code
 *
 * @param rc The result code.
 * @return const char* The string form of the result code.
 */
const char* adu_mqtt_protocol_result_code_str(ADU_RESPONSE_MESSAGE_RESULT_CODE rc)
{
    switch (rc)
    {
    case ADU_RESPONSE_MESSAGE_RESULT_CODE_SUCCESS:
        return "Success";

    case ADU_RESPONSE_MESSAGE_RESULT_CODE_BAD_REQUEST:
        return "Bad Request";

    case ADU_RESPONSE_MESSAGE_RESULT_CODE_BUSY:
        return "Busy";

    case ADU_RESPONSE_MESSAGE_RESULT_CODE_CONFLICT:
        return "Conflict";

    case ADU_RESPONSE_MESSAGE_RESULT_CODE_SERVER_ERROR:
        return "Server Error";

    case ADU_RESPONSE_MESSAGE_RESULT_CODE_AGENT_NOT_ENROLLED:
        return "Agent Not Enrolled";

    default:
        return "???";
    }
}

/**
 * @brief Get the str form of the extended result code
 *
 * @param erc The extended result code.
 * @return const char* The string form of the extended result code.
 */
const char* adu_mqtt_protocol_erc_str(ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE erc)
{
    switch (erc)
    {
    case ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_NONE:
        return "None";

    case ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_UNABLE_TO_PARSE_MESSAGE:
        return "Unable to parse message";

    case ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_MISSING_OR_INVALID_VALUE:
        return "Missing or invalid value";

    case ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_MISSING_OR_INVALID_CORRELATION_ID:
        return "Missing or invalid correlation ID";

    case ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_MISSING_OR_INVALID_MESSAGE_TYPE:
        return "Missing or invalid message type";

    case ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_MISSING_OR_INVALID_PROTOCOL_VERSION:
        return "Missing or invalid protocol version";

    case ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_PROTOCOL_VERSION_MISMATCH:
        return "Protocol versions mismatch";

    case ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_MISSING_OR_INVALID_CONTENT_TYPE:
        return "Missing or invalid content type";

    default:
        return "???";
    }
}
