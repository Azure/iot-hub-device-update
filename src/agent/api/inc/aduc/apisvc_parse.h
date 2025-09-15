#ifndef ADUC_APISVC_H_
#define ADUC_APISVC_H_

typedef enum tagAducApiParseStatus
{
    AducApiParseStatus_Complete = 0,
    AducApiParseStatus_Continue = 1,
    AducApiParseStatus_Fail = 2,
} AducApiParseStatus;

typedef struct tagAducApiCallParseState
{
    uint16_t ver;
    uint16_t cmd;
    uint16_t arg_len;
    ssize_t cur_pos;
    unsigned char* args;
    ssize_t args_len;
} AducApiCallParseState;

bool init_parse_state(AducApiCallParseState* out_state);
void reset_parse_state(AducApiCallParseState* out_state);
void free_parse_state(AducApiCallParseState* out_state);

AducApiParseStatus parse_call(AducApiCallWireState* wire_state, ssize_t start_pos, ssize_t bytes_read, AducApiCallParseState* out_parse_state)
{
    AducApiParseStatus status = AducApiParseStatus_Fail;
    AducApiCall call = {0};
}


#endif // ADUC_APISVC_H_
