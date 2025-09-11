
#ifndef ADUC_APISVC_INTERNAL_H_
#define ADUC_APISVC_INTERNAL_H_

typedef struct tagGetStateRequestArgs
{
    uint16_t response_path_len; ///< The Byte length of the response_path string. NOTE: NOT null-terminated
    char* response_path; ///< UTF-8 encoded string path to the response FIFO (No null terminator)
} GetStateRequestArgs;

typedef enum tagAducApiRequestType
{
    AducApiInstrCode_Unknown = 0,
    AducApiInstrCode_GetState = 1,
} AducApiRequestType;

typedef union tagApiSvcRequestArgs
{
    GetStateRequestArgs get_state_args;
    // Future requests can be added here
} ApiSvcRequestArgs;

typedef struct tagAducApiCall
{
    uint16_t version; ///< Protocol version bytes of the API call structure
    AducApiRequestType req_type; ///< The type of request being made
    ApiSvcRequestArgs req_args; ///< Request arguments for the API call
} AducApiCall;

typedef enum tagADUC_CrossProcApi_RespCode
{
    ADUC_CrossProcApi_RespCode_Success = 0,
    ADUC_CrossProcApi_RespCode_Failure = 1,
} ADUC_CrossProcApi_RespCode;

typedef struct tagADUC_GetStateApiReponse
{
    ADUC_CrossProcApi_RespCode resp_code;
    ADUC_ServiceStatus state;
} ADUC_GetStateApiResponse;

#endif // ADUC_APISVC_INTERNAL_H_
